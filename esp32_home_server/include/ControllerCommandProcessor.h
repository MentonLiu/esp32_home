// 文件说明：esp32_home_server/include/ControllerCommandProcessor.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef CONTROLLER_COMMAND_PROCESSOR_H
#define CONTROLLER_COMMAND_PROCESSOR_H

#include <Arduino.h>

#include "Controllerr.h"
#include "SystemContracts.h"

// 控制指令处理器：
// 负责把 JSON 命令路由到风扇/窗帘/蜂鸣器/红外控制器，并维护统一状态。
class ControllerCommandProcessor
{
public:
    ControllerCommandProcessor(RelayFanController &fan,
                               DualCurtainController &curtain,
                               BuzzerController &buzzer,
                               IRController &ir);

    // 初始化所有执行器，并绑定红外桥接串口。
    void begin(Stream &irBridgeSerial);
    // 解析并执行 JSON 控制命令。
    CommandResult processCommandJson(const String &jsonText, CommandSource source);

    // 风扇电源开关（逻辑层）。
    void setFanPower(bool powerOn);
    // 查询风扇是否处于通电状态。
    bool isFanPowerOn() const;
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
    // 触发火警蜂鸣模式。
    void playFireAlarmPattern();
    // 直接下发红外桥接命令，供自动化规则调用。
    bool sendIrCommand(const String &commandText);

    // 轮询红外桥接回传消息。
    bool pollIrBridgeMessage(String &payload);
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
    IRController &ir_;
    ControllerState state_;
    unsigned long lastManualCurtainCommandMs_ = 0;
};

#endif
