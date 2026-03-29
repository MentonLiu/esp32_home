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
    constexpr unsigned long kRainDetectDebounceMs = 3000UL;
    constexpr unsigned long kRainClearHoldMs = 180000UL;
    constexpr unsigned long kRainCurtainCooldownMs = 60000UL;

    // 将日期压缩为“天序号”，用于每日只触发一次的去重逻辑。
    uint32_t dayStampFromUnix(time_t unixSeconds)
    {
        if (unixSeconds <= 0)
        {
            return 0;
        }
        return static_cast<uint32_t>(static_cast<unsigned long>(unixSeconds) / 86400UL);
    }

    bool isDaytimeWindow(time_t unixSeconds)
    {
        struct tm localTime;
        localtime_r(&unixSeconds, &localTime);
        const uint8_t hour = static_cast<uint8_t>(localTime.tm_hour);
        return hour >= 8 && hour < 18;
    }
} // namespace

AutomationEngine::AutomationEngine(ConnectivityManager &net,
                                   ControllerCommandProcessor &commandProcessor,
                                   StatusReporter statusReporter)
    : net_(net),
      commandProcessor_(commandProcessor),
      statusReporter_(statusReporter)
{
}

void AutomationEngine::begin()
{
    // 记录回退时钟基准点（无 NTP 时使用）。
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
        const time_t nowUnix = currentTime();
        handleCurtainSchedule(nowUnix);
        handleLightAutomation(sensorData, nowUnix);
    }

    // 传感联动可高频检查。
    handleTemperatureAutomation(sensorData);
    handleSmokeAutomation(sensorData);
    handleRainAutomation(sensorData);
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
    // 优先使用 NTP（在线最准确）。
    if (ntpConfigured_)
    {
        const time_t now = time(nullptr);
        if (now > 100000)
        {
            return now;
        }
    }

    // 回退到“内置 Unix 基准 + 开机运行时长”。
    return fallbackBaseUnix_ + static_cast<time_t>((millis() - fallbackBaseMillis_) / 1000UL);
}

void AutomationEngine::handleCurtainSchedule(time_t nowUnix)
{
    const uint32_t dayStamp = dayStampFromUnix(nowUnix);
    struct tm localTime;
    localtime_r(&nowUnix, &localTime);

    // 每日 07:00 自动开窗帘，且同一天只触发一次。
    if (!rainLockActive_ && localTime.tm_hour == 7 && localTime.tm_min == 0 && lastOpenDayStamp_ != dayStamp)
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

void AutomationEngine::handleRainAutomation(const StandardSensorData &sensorData)
{
    const unsigned long nowMs = millis();

    if (sensorData.isRaining)
    {
        rainClearedSinceMs_ = 0;
        if (rainDetectedSinceMs_ == 0)
        {
            rainDetectedSinceMs_ = nowMs;
        }

        if (!rainLockActive_ && nowMs - rainDetectedSinceMs_ >= kRainDetectDebounceMs)
        {
            rainLockActive_ = true;
            publishStatus(mqtt_upstream::statusTopic(), "automation", "rain_detected_lock_curtain");
        }

        // 激活雨滴锁后，周期受限地执行一次关窗帘动作，防止误复位后未关严。
        if (rainLockActive_ && nowMs - lastRainCurtainActionMs_ >= kRainCurtainCooldownMs)
        {
            lastRainCurtainActionMs_ = nowMs;
            commandProcessor_.setCurtainPreset(0);
            publishStatus(mqtt_upstream::statusTopic(), "automation", "curtain_close_by_rain");
        }
        return;
    }

    // 非雨状态时重置检测时间，且仅在锁定状态下记录清除时间。
    rainDetectedSinceMs_ = 0;
    if (!rainLockActive_)
    {
        return;
    }

    // 雨停后需要持续一段时间才解除锁定，避免阈值边界抖动导致联动来回切换。
    if (rainClearedSinceMs_ == 0)
    {
        rainClearedSinceMs_ = nowMs;
        return;
    }

    // 雨停稳定后全开窗帘并释放锁定。
    if (nowMs - rainClearedSinceMs_ >= kRainClearHoldMs)
    {
        rainLockActive_ = false;
        rainClearedSinceMs_ = 0;
        commandProcessor_.setCurtainPreset(4);
        publishStatus(mqtt_upstream::statusTopic(), "automation", "curtain_full_open_after_rain");
        publishStatus(mqtt_upstream::statusTopic(), "automation", "rain_cleared_release_curtain_lock");
    }
}

void AutomationEngine::handleLightAutomation(const StandardSensorData &sensorData, time_t nowUnix)
{
    // 雨滴锁定期间不允许光照联动重新开窗帘。
    if (rainLockActive_)
    {
        return;
    }

    // 仅白天启用光照联动，避免夜间误动作。
    if (!isDaytimeWindow(nowUnix))
    {
        return;
    }

    // 用户手动控制后 30 分钟内不接管窗帘。
    // const unsigned long lastManualMs = commandProcessor_.lastManualCurtainCommandMs();
    // if (lastManualMs > 0 && millis() - lastManualMs < kCurtainManualOverrideMs)
    // {
    //     return;
    // }

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
    // if (lightCurtainMode_ != LightCurtainMode::DaylightBoost && sensorData.lightPercent <= kLowLightOnThresholdPercent)
    // {
    //     lightCurtainMode_ = LightCurtainMode::DaylightBoost;
    //     lastLightCurtainActionMs_ = millis();
    //     commandProcessor_.setCurtainPreset(3);
    //     publishStatus(mqtt_upstream::statusTopic(), "automation", "curtain_daylight_boost");
    //     return;
    // }

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

void AutomationEngine::publishStatus(const char *topic, const String &type, const String &message) const
{
    if (statusReporter_)
    {
        statusReporter_(topic, type, message);
    }
}
