#include "AutomationEngine.h"

#include <Wire.h>
#include <time.h>

#include "MqttUpstream.h"
#include "pins.h"

namespace
{
    // 东八区本地时区偏移。
    constexpr long kUtcOffsetSeconds = 8 * 3600;

    // 按天去重的时间戳，避免同一日重复触发。
    uint32_t dayStampFromDateTime(const DateTime &timePoint)
    {
        return static_cast<uint32_t>(timePoint.unixtime() / 86400UL);
    }
} // 命名空间

AutomationEngine::AutomationEngine(ConnectivityManager &net,
                                   ControllerCommandProcessor &commandProcessor,
                                   StatusReporter statusReporter)
    : net_(net),
      commandProcessor_(commandProcessor),
      statusReporter_(statusReporter),
      fallbackBaseTime_(DateTime(F(__DATE__), F(__TIME__)))
{
}

void AutomationEngine::begin()
{
    // 启动实时时钟总线，并立即记录回退时间基线。
    Wire.begin(pins::RTC_I2C_SDA, pins::RTC_I2C_SCL);
    rtcAvailable_ = rtc_.begin();
    fallbackBaseMillis_ = millis();
    ensureTimeSource();
    syncRtcFromNtp();
}

void AutomationEngine::loop(const StandardSensorData &sensorData)
{
    // 执行定时策略前先确保时间源可用。
    ensureTimeSource();
    syncRtcFromNtp();

    // 分钟级任务使用 1 秒节拍已足够。
    if (millis() - lastScheduleCheckMs_ >= 1000)
    {
        lastScheduleCheckMs_ = millis();
        handleCurtainSchedule(currentTime());
    }

    handleSmokeAutomation(sensorData);
    handleFlameAutomation(sensorData);
}

void AutomationEngine::ensureTimeSource()
{
    // 网络可用后仅配置一次网络授时。
    if (net_.isInternetConnected() && !ntpConfigured_)
    {
        configTime(kUtcOffsetSeconds, 0, "pool.ntp.org", "ntp.aliyun.com", "time.nist.gov");
        ntpConfigured_ = true;
    }
}

void AutomationEngine::syncRtcFromNtp()
{
    // 仅进行一次实时时钟校时，减少写入与损耗。
    if (!rtcAvailable_ || rtcSyncedFromNtp_ || !ntpConfigured_)
    {
        return;
    }

    const time_t now = time(nullptr);
    // 网络授时未锁定前的无效时间戳直接忽略。
    if (now <= 100000)
    {
        return;
    }

    struct tm localTime;
    localtime_r(&now, &localTime);
    rtc_.adjust(DateTime(localTime.tm_year + 1900,
                         localTime.tm_mon + 1,
                         localTime.tm_mday,
                         localTime.tm_hour,
                         localTime.tm_min,
                         localTime.tm_sec));
    rtcSyncedFromNtp_ = true;
    publishStatus(mqtt_upstream::statusTopic(), "rtc", "rtc_synced_from_ntp");
}

DateTime AutomationEngine::currentTime()
{
    // 优先级 1：网络授时同步后的系统时间。
    if (ntpConfigured_)
    {
        const time_t now = time(nullptr);
        if (now > 100000)
        {
            struct tm localTime;
            localtime_r(&now, &localTime);
            return DateTime(localTime.tm_year + 1900,
                            localTime.tm_mon + 1,
                            localTime.tm_mday,
                            localTime.tm_hour,
                            localTime.tm_min,
                            localTime.tm_sec);
        }
    }

    // 优先级 2：实时时钟时间（含年份有效性校验）。
    if (rtcAvailable_)
    {
        const DateTime rtcNow = rtc_.now();
        if (rtcNow.year() >= 2024)
        {
            return rtcNow;
        }
    }

    // 优先级 3：编译时间回退值 + 运行时增量。
    return fallbackBaseTime_ + TimeSpan((millis() - fallbackBaseMillis_) / 1000UL);
}

void AutomationEngine::handleCurtainSchedule(const DateTime &now)
{
    const uint32_t dayStamp = dayStampFromDateTime(now);

    if (now.hour() == 7 && now.minute() == 0 && lastOpenDayStamp_ != dayStamp)
    {
        commandProcessor_.setCurtainPreset(4);
        lastOpenDayStamp_ = dayStamp;
        publishStatus(mqtt_upstream::statusTopic(), "automation", "curtain_open_07_00");
    }

    if (now.hour() == 22 && now.minute() == 0 && lastCloseDayStamp_ != dayStamp)
    {
        commandProcessor_.setCurtainPreset(0);
        lastCloseDayStamp_ = dayStamp;
        publishStatus(mqtt_upstream::statusTopic(), "automation", "curtain_close_22_00");
    }
}

void AutomationEngine::handleSmokeAutomation(const StandardSensorData &sensorData)
{
    // 随烟雾浓度提升风扇档位。
    if (sensorData.mq2Percent >= 75)
    {
        commandProcessor_.setFanMode(FanMode::High);
    }

    // 高烟雾时按冷却间隔播放告警音。
    if (sensorData.mq2Percent >= 90 && millis() - lastHighSmokeBeepMs_ >= 1200)
    {
        lastHighSmokeBeepMs_ = millis();
        commandProcessor_.beep(2600, 100);
    }
}

void AutomationEngine::handleFlameAutomation(const StandardSensorData &sensorData)
{
    // 火焰消失时立即重置升级状态。
    if (!sensorData.flameDetected)
    {
        flameDetectedSinceMs_ = 0;
        fireAlarmReported_ = false;
        return;
    }

    // 记录首次检测到火焰的时刻。
    if (flameDetectedSinceMs_ == 0)
    {
        flameDetectedSinceMs_ = millis();
    }

    // 火焰持续一段时间后，周期性播放本地告警模式。
    const unsigned long durationMs = millis() - flameDetectedSinceMs_;
    if (durationMs >= 45000 && millis() - lastFlamePatternMs_ >= 3000)
    {
        lastFlamePatternMs_ = millis();
        commandProcessor_.playFireAlarmPattern();
    }

    // 云模式下持续 5 分钟后仅上报一次火警。
    if (durationMs >= 300000 && !fireAlarmReported_ && net_.isCloudMode())
    {
        fireAlarmReported_ = true;
        publishStatus(mqtt_upstream::alarmTopic(), "fire", "flame_over_5min_fire_service_alert");
    }
}

void AutomationEngine::publishStatus(const char *topic, const String &type, const String &message) const
{
    if (statusReporter_)
    {
        statusReporter_(topic, type, message);
    }
}
