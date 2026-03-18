/**
 * @file HomeServiceAutomation.cpp
 * @brief HomeService 的自动化、报警与时间管理实现
 */

#include "HomeService.h"

#include <time.h>

namespace
{
    const char *kStatusTopic = "esp32/home/status";
    const char *kAlarmTopic = "esp32/home/alarm";
}

void HomeService::handleAutomation()
{
    if (millis() - lastAutomationMs_ < 1000)
    {
        return;
    }
    lastAutomationMs_ = millis();

    if (net_.isInternetConnected() && !ntpConfigured_)
    {
        configTime(8 * 3600, 0, "pool.ntp.org", "ntp.aliyun.com");
        ntpConfigured_ = true;
    }

    syncRtcFromNtpIfNeeded();

    const int hour = currentHour();
    const int minute = currentMinute();
    const uint32_t dayIndex = currentDayIndex();

    if (hour == 7 && minute == 0 && lastOpenDay_ != dayIndex)
    {
        curtain_.setPresetLevel(4);
        lastOpenDay_ = dayIndex;
        publishStatus(kStatusTopic, "automation", "curtain_open_07_00");
    }

    if (hour == 22 && minute == 0 && lastCloseDay_ != dayIndex)
    {
        curtain_.setPresetLevel(0);
        lastCloseDay_ = dayIndex;
        publishStatus(kStatusTopic, "automation", "curtain_close_22_00");
    }
}

void HomeService::handleAlerts()
{
    const SensorSnapshot &s = sensors_.snapshot();

    if (s.mq2Percent >= 75)
    {
        fan_.setMode(FanMode::High);
    }

    if (s.mq2Percent >= 90 && millis() - lastHighSmokeBeepMs_ >= 1000)
    {
        lastHighSmokeBeepMs_ = millis();
        buzzer_.beep(2600, 80);
    }

    if (s.flameDetected)
    {
        if (flameDetectedSinceMs_ == 0)
        {
            flameDetectedSinceMs_ = millis();
            fireAlarmReported_ = false;
        }

        const unsigned long flameDuration = millis() - flameDetectedSinceMs_;
        if (flameDuration >= 45000 && millis() - lastFlamePatternMs_ >= 2500)
        {
            lastFlamePatternMs_ = millis();
            buzzer_.patternShortShortLong();
        }

        if (flameDuration >= 300000 && !fireAlarmReported_ && net_.isCloudMode())
        {
            fireAlarmReported_ = true;
            publishStatus(kAlarmTopic, "fire", "flame_over_5min_fire_service_alert");
        }
    }
    else
    {
        flameDetectedSinceMs_ = 0;
        fireAlarmReported_ = false;
    }
}

int HomeService::currentHour()
{
    if (ntpConfigured_)
    {
        time_t now = time(nullptr);
        if (now > 100000)
        {
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            return timeinfo.tm_hour;
        }
    }

    if (rtcAvailable_)
    {
        const DateTime rtcNow = rtc_.now();
        if (rtcNow.year() >= 2024)
        {
            return rtcNow.hour();
        }
    }

    const uint32_t seconds = bootBaseSeconds_ + millis() / 1000;
    return (seconds / 3600) % 24;
}

int HomeService::currentMinute()
{
    if (ntpConfigured_)
    {
        time_t now = time(nullptr);
        if (now > 100000)
        {
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            return timeinfo.tm_min;
        }
    }

    if (rtcAvailable_)
    {
        const DateTime rtcNow = rtc_.now();
        if (rtcNow.year() >= 2024)
        {
            return rtcNow.minute();
        }
    }

    const uint32_t seconds = bootBaseSeconds_ + millis() / 1000;
    return (seconds / 60) % 60;
}

uint32_t HomeService::currentDayIndex()
{
    if (ntpConfigured_)
    {
        time_t now = time(nullptr);
        if (now > 100000)
        {
            return static_cast<uint32_t>(now / 86400);
        }
    }

    if (rtcAvailable_)
    {
        const DateTime rtcNow = rtc_.now();
        if (rtcNow.year() >= 2024)
        {
            return rtcNow.unixtime() / 86400;
        }
    }

    const uint32_t seconds = bootBaseSeconds_ + millis() / 1000;
    return seconds / 86400;
}

void HomeService::syncRtcFromNtpIfNeeded()
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
    const DateTime localDateTime(localTime.tm_year + 1900,
                                 localTime.tm_mon + 1,
                                 localTime.tm_mday,
                                 localTime.tm_hour,
                                 localTime.tm_min,
                                 localTime.tm_sec);

    rtc_.adjust(localDateTime);
    rtcSyncedFromNtp_ = true;
    publishStatus(kStatusTopic, "rtc", "rtc_synced_from_ntp");
}
