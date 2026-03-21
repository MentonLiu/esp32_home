// 文件说明：esp32_home_server/include/CentralProcessor.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef CENTRAL_PROCESSOR_H
#define CENTRAL_PROCESSOR_H

#include <Arduino.h>

#include "AutomationEngine.h"
#include "ConnectivityManager.h"
#include "ControllerCommandProcessor.h"
#include "LocalProcessingProgram.h"
#include "Sensor.h"
#include "SensorDataProcessor.h"

class CentralProcessor
{
public:
    CentralProcessor();

    void begin();
    void loop();

private:
    void publishStatus(const char *topic, const String &type, const String &message);

    SensorHub sensorHub_;
    RelayFanController fan_;
    DualCurtainController curtain_;
    BuzzerController buzzer_;
    IRController ir_;
    ConnectivityManager net_;

    SensorDataProcessor sensorDataProcessor_;
    ControllerCommandProcessor commandProcessor_;
    AutomationEngine automationEngine_;
    LocalProcessingProgram localProgram_;
};

#endif
