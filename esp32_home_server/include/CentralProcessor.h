/**
 * 文件：esp32_home_server/include/CentralProcessor.h
 * 功能说明：
 *   - 系统的唯一编排入口
 *   - 负责所有模块的构造顺序、初始化顺序和循环调度
 *   - 维护依赖关系：Sensor → SensorDataProcessor → AutomationEngine → 执行器
 *   - 维护网络循环：ConnectivityManager、LocalProcessingProgram、AutomationEngine
 *
 * 核心方法：
 *   - begin() - 初始化所有子模块（有特定顺序）
 *   - loop() - 主循环：驱动所有子模块的 loop() 方法
 *
 * 模块依赖关系：
 *   - Sensor → SensorDataProcessor → AutomationEngine
 *   - Sensor → SensorDataProcessor → LocalProcessingProgram (/api/status)
 *   - SensorHub + Controllerr → ControllerCommandProcessor
 *   - ControllerCommandProcessor → AutomationEngine（执行联动）
 *   - ControllerCommandProcessor → LocalProcessingProgram (/api/control)
 *   - ConnectivityManager → AutomationEngine（时间同步、MQTT 发布）
 *   - ConnectivityManager → LocalProcessingProgram（WebServer 驱动）
 *
 * 依赖：所有其他模块（双向依赖中心）
 * 被依赖于：main.cpp
 */

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
    ConnectivityManager net_;

    // 流程层模块实例。
    SensorDataProcessor sensorDataProcessor_;
    ControllerCommandProcessor commandProcessor_;
    AutomationEngine automationEngine_;
    LocalProcessingProgram localProgram_;
};

#endif
