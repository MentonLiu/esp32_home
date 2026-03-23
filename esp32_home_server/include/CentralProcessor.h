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

// 中央处理器：
// 系统唯一编排入口，负责模块构造顺序、初始化顺序和主循环调度。
class CentralProcessor
{
public:
    CentralProcessor();

    // 完成系统级初始化。
    void begin();
    // 驱动系统主循环。
    void loop();

private:
    // 状态上报统一出口（当前实现主要输出到串口）。
    void publishStatus(const char *topic, const String &type, const String &message);

    // 底层模块实例。
    SensorHub sensorHub_;
    RelayFanController fan_;
    DualCurtainController curtain_;
    BuzzerController buzzer_;
    IRController ir_;
    ConnectivityManager net_;

    // 流程层模块实例。
    SensorDataProcessor sensorDataProcessor_;
    ControllerCommandProcessor commandProcessor_;
    AutomationEngine automationEngine_;
    LocalProcessingProgram localProgram_;
};

#endif
