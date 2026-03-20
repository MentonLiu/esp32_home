#ifndef CENTRAL_PROCESSOR_H
#define CENTRAL_PROCESSOR_H

#include <Arduino.h>

#include "AutomationEngine.h"
#include "ConnectivityManager.h"
#include "ControllerCommandProcessor.h"
#include "LocalProcessingProgram.h"
#include "MqttUpstream.h"
#include "Sensor.h"
#include "SensorDataProcessor.h"

class CentralProcessor
{
public:
    CentralProcessor();

    void begin();
    void loop();

private:
    void handleCloudCommand(char *topic, uint8_t *payload, unsigned int length);
    void publishStatus(const char *topic, const String &type, const String &message);

    SensorHub sensorHub_;
    RelayFanController fan_;
    DualCurtainController curtain_;
    BuzzerController buzzer_;
    IRController ir_;
    ConnectivityManager net_;

    SensorDataProcessor sensorDataProcessor_;
    ControllerCommandProcessor commandProcessor_;
    LocalProcessingProgram localProgram_;
    AutomationEngine automationEngine_;
};

#endif
