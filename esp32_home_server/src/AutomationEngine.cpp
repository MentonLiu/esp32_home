// 文件说明：esp32_home_server/src/AutomationEngine.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "AutomationEngine.h"

#include <Wire.h>
#include <time.h>

#include "MqttUpstream.h"
#include "pins.h"

namespace
{
    constexpr long kUtcOffsetSeconds = 8 * 3600;

    constexpr uint8_t kTempFanOnThresholdC = 32;
    constexpr uint8_t kTempFanOffThresholdC = 29;
    constexpr unsigned long kTempFanCooldownMs = 10UL * 60UL * 1000UL;

    constexpr uint8_t kBrightLightOnThresholdPercent = 85;
    constexpr uint8_t kBrightLightOffThresholdPercent = 70;
    constexpr uint8_t kLowLightOnThresholdPercent = 30;
    constexpr uint8_t kLowLightOffThresholdPercent = 45;
    constexpr unsigned long kLightCurtainCooldownMs = 10UL * 60UL * 1000UL;
    constexpr unsigned long kCurtainManualOverrideMs = 30UL * 60UL * 1000UL;

    constexpr uint8_t kIrTempTriggerC = 30;
    constexpr uint8_t kIrTempRecoverC = 27;
    constexpr uint8_t kIrHumidityTriggerPercent = 80;
    constexpr uint8_t kIrHumidityRecoverPercent = 70;
    constexpr unsigned long kIrTempHoldMs = 5UL * 60UL * 1000UL;
    constexpr unsigned long kIrHumidityHoldMs = 10UL * 60UL * 1000UL;
    constexpr unsigned long kIrActionCooldownMs = 15UL * 60UL * 1000UL;

    // 将日期压缩为“天序号”，用于每日只触发一次的去重逻辑。
    uint32_t dayStampFromDateTime(const DateTime &timePoint)
    {
        return static_cast<uint32_t>(timePoint.unixtime() / 86400UL);
    }

    bool isDaytimeWindow(const DateTime &timePoint)
    {
        const uint8_t hour = timePoint.hour();
        return hour >= 8 && hour < 18;
    }

    bool isNightWindow(const DateTime &timePoint)
    {
        const uint8_t hour = timePoint.hour();
        return hour >= 22 || hour < 6;
    }
} // namespace

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
    // 初始化 RTC 所在 I2C 总线。
    Wire.begin(pins::RTC_I2C_SDA, pins::RTC_I2C_SCL);
    rtcAvailable_ = rtc_.begin();
    // 记录回退时钟基准点。
    fallbackBaseMillis_ = millis();
    ensureTimeSource();
    syncRtcFromNtp();
}

void AutomationEngine::loop(const StandardSensorData &sensorData)
{
    // 每轮循环都尝试维护时间源状态。
    ensureTimeSource();
    syncRtcFromNtp();

    // 定时任务按 1Hz 执行即可，避免无意义高频计算。
    if (millis() - lastScheduleCheckMs_ >= 1000)
    {
        lastScheduleCheckMs_ = millis();
        const DateTime now = currentTime();
        handleCurtainSchedule(now);
        handleLightAutomation(sensorData, now);
        handleClimateIrAutomation(sensorData, now);
    }

    // 传感联动可高频检查。
    handleTemperatureAutomation(sensorData);
    handleSmokeAutomation(sensorData);
    handleFlameAutomation(sensorData);
}

void AutomationEngine::ensureTimeSource()
{
    if (net_.isInternetConnected() && !ntpConfigured_)
    {
        // 联网后配置多个 NTP 源，提升可用性。
        configTime(kUtcOffsetSeconds, 0, "pool.ntp.org", "ntp.aliyun.com", "time.nist.gov");
        ntpConfigured_ = true;
    }
}

void AutomationEngine::syncRtcFromNtp()
{
    // 只在“RTC 可用 + NTP 已配置 + 尚未同步”时执行一次。
    if (!rtcAvailable_ || rtcSyncedFromNtp_ || !ntpConfigured_)
    {
        return;
    }

    const time_t now = time(nullptr);
    // NTP 尚未获取到有效时间时，time 可能过小。
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
    // 记录同步事件，便于排查离线时间异常。
    publishStatus(mqtt_upstream::statusTopic(), "rtc", "rtc_synced_from_ntp");
}

DateTime AutomationEngine::currentTime()
{
    // 优先使用 NTP（在线最准确）。
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

    if (rtcAvailable_)
    {
        const DateTime rtcNow = rtc_.now();
        // 对 RTC 年份做最小有效值保护，避免未校时数据污染自动化。
        if (rtcNow.year() >= 2024)
        {
            return rtcNow;
        }
    }

    // 最后回退到“编译时刻 + 开机已运行时长”。
    return fallbackBaseTime_ + TimeSpan((millis() - fallbackBaseMillis_) / 1000UL);
}

void AutomationEngine::handleCurtainSchedule(const DateTime &now)
{
    const uint32_t dayStamp = dayStampFromDateTime(now);

    // 每日 07:00 自动开窗帘，且同一天只触发一次。
    if (now.hour() == 7 && now.minute() == 0 && lastOpenDayStamp_ != dayStamp)
    {
        commandProcessor_.setCurtainPreset(4);
        lastOpenDayStamp_ = dayStamp;
        publishStatus(mqtt_upstream::statusTopic(), "automation", "curtain_open_07_00");
    }

    // 每日 22:00 自动关窗帘，且同一天只触发一次。
    if (now.hour() == 22 && now.minute() == 0 && lastCloseDayStamp_ != dayStamp)
    {
        commandProcessor_.setCurtainPreset(0);
        lastCloseDayStamp_ = dayStamp;
        publishStatus(mqtt_upstream::statusTopic(), "automation", "curtain_close_22_00");
    }
}

void AutomationEngine::handleTemperatureAutomation(const StandardSensorData &sensorData)
{
    // 温度阈值滞回：高温触发，高温解除后才退出状态。
    if (!tempFanBoostActive_ && sensorData.temperatureC >= static_cast<float>(kTempFanOnThresholdC))
    {
        if (millis() - lastTempFanActionMs_ >= kTempFanCooldownMs)
        {
            lastTempFanActionMs_ = millis();
            tempFanBoostActive_ = true;
            commandProcessor_.setFanPower(true);
            commandProcessor_.setFanMode(FanMode::High);
            publishStatus(mqtt_upstream::statusTopic(), "automation", "fan_high_by_temperature");
        }
        return;
    }

    if (tempFanBoostActive_ && sensorData.temperatureC <= static_cast<float>(kTempFanOffThresholdC))
    {
        tempFanBoostActive_ = false;
        publishStatus(mqtt_upstream::statusTopic(), "automation", "temperature_back_to_normal");
    }
}

void AutomationEngine::handleSmokeAutomation(const StandardSensorData &sensorData)
{
    // 烟雾达到阈值时自动打开风扇并设为高档。
    if (sensorData.mq2Percent >= 75)
    {
        commandProcessor_.setFanPower(true);
        commandProcessor_.setFanMode(FanMode::High);
    }

    // 高烟雾区间周期性短鸣提醒。
    if (sensorData.mq2Percent >= 90 && millis() - lastHighSmokeBeepMs_ >= 1200)
    {
        lastHighSmokeBeepMs_ = millis();
        commandProcessor_.beep(2600, 100);
    }
}

void AutomationEngine::handleLightAutomation(const StandardSensorData &sensorData, const DateTime &now)
{
    // 仅白天启用光照联动，避免夜间误动作。
    if (!isDaytimeWindow(now))
    {
        return;
    }

    // 用户手动控制后 30 分钟内不接管窗帘。
    const unsigned long lastManualMs = commandProcessor_.lastManualCurtainCommandMs();
    if (lastManualMs > 0 && millis() - lastManualMs < kCurtainManualOverrideMs)
    {
        return;
    }

    // 触发动作冷却，防止频繁摆动。
    if (millis() - lastLightCurtainActionMs_ < kLightCurtainCooldownMs)
    {
        return;
    }

    // 强光时收拢窗帘到 1/4。
    if (lightCurtainMode_ != LightCurtainMode::AntiGlare && sensorData.lightPercent >= kBrightLightOnThresholdPercent)
    {
        lightCurtainMode_ = LightCurtainMode::AntiGlare;
        lastLightCurtainActionMs_ = millis();
        commandProcessor_.setCurtainPreset(1);
        publishStatus(mqtt_upstream::statusTopic(), "automation", "curtain_anti_glare");
        return;
    }

    // 低光时打开到 3/4。
    if (lightCurtainMode_ != LightCurtainMode::DaylightBoost && sensorData.lightPercent <= kLowLightOnThresholdPercent)
    {
        lightCurtainMode_ = LightCurtainMode::DaylightBoost;
        lastLightCurtainActionMs_ = millis();
        commandProcessor_.setCurtainPreset(3);
        publishStatus(mqtt_upstream::statusTopic(), "automation", "curtain_daylight_boost");
        return;
    }

    // 亮度回到中间区间后释放模式锁定，允许下一次联动。
    if (lightCurtainMode_ == LightCurtainMode::AntiGlare && sensorData.lightPercent <= kBrightLightOffThresholdPercent)
    {
        lightCurtainMode_ = LightCurtainMode::Neutral;
        return;
    }

    if (lightCurtainMode_ == LightCurtainMode::DaylightBoost && sensorData.lightPercent >= kLowLightOffThresholdPercent)
    {
        lightCurtainMode_ = LightCurtainMode::Neutral;
    }
}

void AutomationEngine::handleClimateIrAutomation(const StandardSensorData &sensorData, const DateTime &now)
{
    const unsigned long nowMs = millis();

    const bool highTemp = sensorData.temperatureC >= static_cast<float>(kIrTempTriggerC);
    if (highTemp)
    {
        if (highTempSinceMs_ == 0)
        {
            highTempSinceMs_ = nowMs;
        }
    }
    else
    {
        highTempSinceMs_ = 0;
    }

    const bool highHumidityAtNight = isNightWindow(now) && sensorData.humidityPercent >= static_cast<float>(kIrHumidityTriggerPercent);
    if (highHumidityAtNight)
    {
        if (highHumiditySinceMs_ == 0)
        {
            highHumiditySinceMs_ = nowMs;
        }
    }
    else
    {
        highHumiditySinceMs_ = 0;
    }

    const bool tempConditionReady = highTempSinceMs_ > 0 && (nowMs - highTempSinceMs_ >= kIrTempHoldMs);
    const bool humidityConditionReady = highHumiditySinceMs_ > 0 && (nowMs - highHumiditySinceMs_ >= kIrHumidityHoldMs);

    if (!irCoolingActive_ && (tempConditionReady || humidityConditionReady) && (nowMs - lastIrActionMs_ >= kIrActionCooldownMs))
    {
        const bool modeOk = commandProcessor_.sendIrCommand("ac_mode");
        const bool tempDownOk = commandProcessor_.sendIrCommand("ac_temp_down");

        if (modeOk || tempDownOk)
        {
            irCoolingActive_ = true;
            lastIrActionMs_ = nowMs;
            publishStatus(mqtt_upstream::statusTopic(),
                          "automation",
                          tempConditionReady ? "ir_cooling_by_high_temperature" : "ir_cooling_by_night_humidity");
        }
    }

    if (irCoolingActive_ &&
        sensorData.temperatureC <= static_cast<float>(kIrTempRecoverC) &&
        sensorData.humidityPercent <= static_cast<float>(kIrHumidityRecoverPercent))
    {
        irCoolingActive_ = false;
        publishStatus(mqtt_upstream::statusTopic(), "automation", "ir_cooling_condition_recovered");
    }
}

void AutomationEngine::handleFlameAutomation(const StandardSensorData &sensorData)
{
    if (!sensorData.flameDetected)
    {
        // 火焰解除后重置状态，允许下一次完整告警流程。
        flameDetectedSinceMs_ = 0;
        fireAlarmReported_ = false;
        return;
    }

    if (flameDetectedSinceMs_ == 0)
    {
        flameDetectedSinceMs_ = millis();
    }

    const unsigned long durationMs = millis() - flameDetectedSinceMs_;
    // 持续 45 秒后每 3 秒播放一次火警蜂鸣模式。
    if (durationMs >= 45000 && millis() - lastFlamePatternMs_ >= 3000)
    {
        lastFlamePatternMs_ = millis();
        commandProcessor_.playFireAlarmPattern();
    }

    // 持续 5 分钟且云模式下，仅上报一次远端火警。
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
