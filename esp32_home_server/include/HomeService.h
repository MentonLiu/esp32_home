#ifndef HOME_SERVICE_H
#define HOME_SERVICE_H

#include <Arduino.h>

#include "ConnectivityManager.h"
#include "Controllerr.h"
#include "Sensor.h"

class HomeService
{
public:
    HomeService();

    void begin();
    void loop();

private:
    void setupWebRoutes();
    void processControlCommand(const String &jsonText);
    void handleSensorAndPublish();
    void handleAutomation();
    void handleAlerts();
    void publishStatus(const char *topic, const String &type, const String &message) const;
    String buildStatusJson(bool includeMeta = true) const;
    int currentHour() const;
    int currentMinute() const;
    uint32_t currentDayIndex() const;

    SensorHub sensors_;
    RelayFanController fan_;
    DualCurtainController curtain_;
    BuzzerController buzzer_;
    IRController ir_;
    ConnectivityManager net_;

    unsigned long lastSensorPublishMs_ = 0;
    unsigned long lastAutomationMs_ = 0;
    unsigned long flameDetectedSinceMs_ = 0;
    unsigned long lastHighSmokeBeepMs_ = 0;
    unsigned long lastFlamePatternMs_ = 0;

    uint32_t lastOpenDay_ = UINT32_MAX;
    uint32_t lastCloseDay_ = UINT32_MAX;
    bool fireAlarmReported_ = false;

    uint32_t bootBaseSeconds_ = 0;
    bool ntpConfigured_ = false;
};

#endif