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

// 系统组合根：负责模块装配与顶层生命周期驱动。
class CentralProcessor
{
public:
    CentralProcessor();

    void begin();
    void loop();

private:
    // 云端控制主题回调。
    void handleCloudCommand(char *topic, uint8_t *payload, unsigned int length);
    // 供子模块复用的统一状态发布函数。
    void publishStatus(const char *topic, const String &type, const String &message);

    // 硬件与网络层对象。
    SensorHub sensorHub_;
    RelayFanController fan_;
    DualCurtainController curtain_;
    BuzzerController buzzer_;
    IRController ir_;
    ConnectivityManager net_;

    // 领域服务对象。
    SensorDataProcessor sensorDataProcessor_;
    ControllerCommandProcessor commandProcessor_;
    LocalProcessingProgram localProgram_;
    AutomationEngine automationEngine_;
};

#endif
