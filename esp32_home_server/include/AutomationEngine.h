#ifndef AUTOMATION_ENGINE_H
#define AUTOMATION_ENGINE_H

#include <Arduino.h>
#include <RTClib.h>

#include "ConnectivityManager.h"
#include "ControllerCommandProcessor.h"
#include "SystemContracts.h"

class AutomationEngine
{
public:
    AutomationEngine(ConnectivityManager &net,
                     ControllerCommandProcessor &commandProcessor,
                     StatusReporter statusReporter);

    void begin();
    void loop(const StandardSensorData &sensorData);

private:
    void ensureTimeSource();
    void syncRtcFromNtp();
    DateTime currentTime();
    void handleCurtainSchedule(const DateTime &now);
    void handleSmokeAutomation(const StandardSensorData &sensorData);
    void handleFlameAutomation(const StandardSensorData &sensorData);
    void publishStatus(const char *topic, const String &type, const String &message) const;

    ConnectivityManager &net_;
    ControllerCommandProcessor &commandProcessor_;
    StatusReporter statusReporter_;

    RTC_DS3231 rtc_;
    bool rtcAvailable_ = false;
    bool ntpConfigured_ = false;
    bool rtcSyncedFromNtp_ = false;
    DateTime fallbackBaseTime_;
    unsigned long fallbackBaseMillis_ = 0;

    unsigned long lastScheduleCheckMs_ = 0;
    unsigned long flameDetectedSinceMs_ = 0;
    unsigned long lastHighSmokeBeepMs_ = 0;
    unsigned long lastFlamePatternMs_ = 0;
    uint32_t lastOpenDayStamp_ = UINT32_MAX;
    uint32_t lastCloseDayStamp_ = UINT32_MAX;
    bool fireAlarmReported_ = false;
};

#endif
