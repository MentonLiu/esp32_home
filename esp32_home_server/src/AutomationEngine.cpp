// 文件说明：esp32_home_server/src/AutomationEngine.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "AutomationEngine.h"

#include <time.h>

#include "MqttUpstream.h"

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

    int monthFromAbbrev(const char *month)
    {
        static constexpr char kMonths[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
        const char *pos = strstr(kMonths, month);
        if (pos == nullptr)
        {
            return 0;
        }
        return static_cast<int>((pos - kMonths) / 3);
    }

    time_t buildTimeToEpoch()
    {
        // __DATE__ 形如 "Mmm dd yyyy"，__TIME__ 形如 "hh:mm:ss"。
        struct tm buildTm = {};
        char mon[4] = {0};
        int day = 1;
        int year = 1970;
        int hour = 0;
        int minute = 0;
        int second = 0;

        if (sscanf(__DATE__ " " __TIME__, "%3s %d %d %d:%d:%d", mon, &day, &year, &hour, &minute, &second) != 6)
        {
            return 0;
        }

        buildTm.tm_mon = monthFromAbbrev(mon);
        buildTm.tm_mday = day;
        buildTm.tm_year = year - 1900;
        buildTm.tm_hour = hour;
        buildTm.tm_min = minute;
        buildTm.tm_sec = second;
        buildTm.tm_isdst = -1;
        return mktime(&buildTm);
    }

    bool toLocalTime(time_t epoch, struct tm &localTime)
    {
        return localtime_r(&epoch, &localTime) != nullptr;
    }

    // 将日期压缩为“天序号”，用于每日只触发一次的去重逻辑。
    uint32_t dayStampFromEpoch(time_t epoch)
    {
        return static_cast<uint32_t>(epoch / 86400UL);
    }

    bool isDaytimeWindow(const struct tm &localTime)
    {
        const int hour = localTime.tm_hour;
        return hour >= 8 && hour < 18;
    }
} // namespace

AutomationEngine::AutomationEngine(ConnectivityManager &net,
                                   ControllerCommandProcessor &commandProcessor,
                                   StatusReporter statusReporter)
    : net_(net),
      commandProcessor_(commandProcessor),
      statusReporter_(statusReporter),
      fallbackBaseEpoch_(buildTimeToEpoch())
{
}

void AutomationEngine::begin()
{
    // 记录回退时钟基准点。
    fallbackBaseMillis_ = millis();
    ensureTimeSource();
}

void AutomationEngine::loop(const StandardSensorData &sensorData)
{
    // 每轮循环都尝试维护时间源状态。
    ensureTimeSource();

    // 定时任务按 1Hz 执行即可，避免无意义高频计算。
    if (millis() - lastScheduleCheckMs_ >= 1000)
    {
        lastScheduleCheckMs_ = millis();
        const time_t nowEpoch = currentTime();
        handleCurtainSchedule(nowEpoch);
        handleLightAutomation(sensorData, nowEpoch);
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

time_t AutomationEngine::currentTime()
{
    // 优先使用 NTP（在线最准确，time 值有效）。
    if (ntpConfigured_)
    {
        const time_t now = time(nullptr);
        if (now > 100000)
        {
            return now;
        }
    }

    // 最后回退到“编译时刻 + 开机已运行时长”。
    return fallbackBaseEpoch_ + static_cast<time_t>((millis() - fallbackBaseMillis_) / 1000UL);
}

void AutomationEngine::handleCurtainSchedule(time_t nowEpoch)
{
    struct tm localTime;
    if (!toLocalTime(nowEpoch, localTime))
    {
        return;
    }
    const uint32_t dayStamp = dayStampFromEpoch(nowEpoch);

    // 每日 07:00 自动开窗帘，且同一天只触发一次。
    if (localTime.tm_hour == 7 && localTime.tm_min == 0 && lastOpenDayStamp_ != dayStamp)
    {
        commandProcessor_.setCurtainPreset(4);
        lastOpenDayStamp_ = dayStamp;
        publishStatus(mqtt_upstream::statusTopic(), "automation", "curtain_open_07_00");
    }

    // 每日 22:00 自动关窗帘，且同一天只触发一次。
    if (localTime.tm_hour == 22 && localTime.tm_min == 0 && lastCloseDayStamp_ != dayStamp)
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
        commandProcessor_.setFanMode(FanMode::High);
    }

    // 高烟雾区间周期性短鸣提醒。
    if (sensorData.mq2Percent >= 90 && millis() - lastHighSmokeBeepMs_ >= 1200)
    {
        lastHighSmokeBeepMs_ = millis();
        commandProcessor_.beep(2400, 260);
    }
}

void AutomationEngine::handleLightAutomation(const StandardSensorData &sensorData, time_t nowEpoch)
{
    struct tm localTime;
    if (!toLocalTime(nowEpoch, localTime))
    {
        return;
    }

    // 仅白天启用光照联动，避免夜间误动作。
    if (!isDaytimeWindow(localTime))
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
