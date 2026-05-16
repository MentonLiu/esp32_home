/**
 * 文件：esp32_home_server/include/ControllerCommandProcessor.h
 * 功能说明：
 *   - 解析并转发 JSON 控制命令到风扇、窗帘、蜂鸣器驱动器
 *   - 维护聚合的控制器状态快照（ControllerState）
 *   - 跟踪手动窗帘控制时间戳（用于自动化联动的手动覆盖逻辑）
 *   - 提供多种便捷方法直接控制执行器
 *
 * 核心方法：
 *   - processCommandJson() - 解析 JSON 命令，执行并返回结果
 *   - setFanMode/setFanSpeedPercent() - 风扇控制
 *   - setCurtainAngle/setCurtainPreset() - 窗帘控制
 *   - beep() - 蜂鸣器控制
 *   - state() - 获取当前状态快照
 *
 * 依赖：Controllerr.h, SystemContracts.h, Logger.h, ArduinoJson 库
 * 被依赖于：ConnectivityManager.h, AutomationEngine.h, LocalProcessingProgram.h
 *
 * 设计细节：
 *   - 支持三类命令来源：LocalWeb、CloudMqtt、Automation（用于审计）
 *   - 手动窗帘控制时间戳用于判断是否在手动覆盖周期内
 *   - 所有控制方法都通过回读硬件驱动器状态来更新内部状态
 */

#ifndef CONTROLLER_COMMAND_PROCESSOR_H
#define CONTROLLER_COMMAND_PROCESSOR_H

#include <Arduino.h>

#include "Controllerr.h"
#include "SystemContracts.h"

// 控制指令处理器：
// 负责把 JSON 命令路由到风扇/窗帘/蜂鸣器控制器，并维护统一状态。
class ControllerCommandProcessor
{
public:
    ControllerCommandProcessor(RelayFanController &fan,
                               DualCurtainController &curtain,
                               BuzzerController &buzzer);

    // 初始化所有执行器。
    void begin();
    // 推进执行器内部的非阻塞状态机。
    void loop();
    // 解析并执行 JSON 控制命令。
    CommandResult processCommandJson(const String &jsonText, CommandSource source);

    // 通过档位设置风扇速度。
    void setFanMode(FanMode mode);
    // 通过百分比设置风扇速度。
    void setFanSpeedPercent(uint8_t speedPercent);
    // 设置窗帘角度。
    void setCurtainAngle(uint8_t angle);
    // 设置窗帘预设。
    void setCurtainPreset(uint8_t preset);
    // 触发蜂鸣器单次鸣叫。
    void beep(uint16_t frequency, uint16_t durationMs);
    // 获取当前控制器状态快照。
    const ControllerState &state() const;
    // 最近一次手动窗帘控制时间戳（毫秒）。
    unsigned long lastManualCurtainCommandMs() const;

private:
    // 从各控制器回读真实状态，更新聚合状态。
    void refreshState();
    // 将命令来源转换为协议文本。
    const char *sourceType(CommandSource source) const;

    RelayFanController &fan_;
    DualCurtainController &curtain_;
    BuzzerController &buzzer_;
    ControllerState state_;
    unsigned long lastManualCurtainCommandMs_ = 0;
};

#endif
