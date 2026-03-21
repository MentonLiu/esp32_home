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

    uint32_t dayStampFromDateTime(const DateTime &timePoint)
    {
        return static_cast<uint32_t>(timePoint.unixtime() / 86400UL);
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
    Wire.begin(pins::RTC_I2C_SDA, pins::RTC_I2C_SCL);
    rtcAvailable_ = rtc_.begin();
    fallbackBaseMillis_ = millis();
    ensureTimeSource();
    syncRtcFromNtp();
}

void AutomationEngine::loop(const StandardSensorData &sensorData)
{
    ensureTimeSource();
    syncRtcFromNtp();

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
    if (net_.isInternetConnected() && !ntpConfigured_)
    {
        configTime(kUtcOffsetSeconds, 0, "pool.ntp.org", "ntp.aliyun.com", "time.nist.gov");
        ntpConfigured_ = true;
    }
}

void AutomationEngine::syncRtcFromNtp()
{
    if (!rtcAvailable_ || rtcSyncedFromNtp_ || !ntpConfigured_)
    {
        return;
    }

    const time_t now = time(nullptr);
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
        if (rtcNow.year() >= 2024)
        {
            return rtcNow;
        }
    }

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
    if (sensorData.mq2Percent >= 75)
    {
        commandProcessor_.setFanPower(true);
        commandProcessor_.setFanMode(FanMode::High);
    }

    if (sensorData.mq2Percent >= 90 && millis() - lastHighSmokeBeepMs_ >= 1200)
    {
        lastHighSmokeBeepMs_ = millis();
        commandProcessor_.beep(2600, 100);
    }
}

void AutomationEngine::handleFlameAutomation(const StandardSensorData &sensorData)
{
    if (!sensorData.flameDetected)
    {
        flameDetectedSinceMs_ = 0;
        fireAlarmReported_ = false;
        return;
    }

    if (flameDetectedSinceMs_ == 0)
    {
        flameDetectedSinceMs_ = millis();
    }

    const unsigned long durationMs = millis() - flameDetectedSinceMs_;
    if (durationMs >= 45000 && millis() - lastFlamePatternMs_ >= 3000)
    {
        lastFlamePatternMs_ = millis();
        commandProcessor_.playFireAlarmPattern();
    }

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
