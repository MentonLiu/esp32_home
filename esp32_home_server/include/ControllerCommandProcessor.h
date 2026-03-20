#ifndef CONTROLLER_COMMAND_PROCESSOR_H
#define CONTROLLER_COMMAND_PROCESSOR_H

#include <Arduino.h>

#include "Controllerr.h"
#include "SystemContracts.h"

// 解析控制文本指令，并分发到具体执行器控制器。
class ControllerCommandProcessor
{
public:
    ControllerCommandProcessor(RelayFanController &fan,
                               DualCurtainController &curtain,
                               BuzzerController &buzzer,
                               IRController &ir);

    void begin(Stream &irBridgeSerial);
    CommandResult processCommandJson(const String &jsonText, CommandSource source);

    void setFanMode(FanMode mode);
    void setFanSpeedPercent(uint8_t speedPercent);
    void setCurtainAngle(uint8_t angle);
    void setCurtainPreset(uint8_t preset);
    void beep(uint16_t frequency, uint16_t durationMs);
    void playFireAlarmPattern();

    // 轮询红外桥接模块的异步入站数据。
    bool pollIrBridgeMessage(String &payload);
    const ControllerState &state() const;

private:
    // 重建统一控制器状态快照。
    void refreshState();
    // 将命令来源枚举映射为状态消息类型。
    const char *sourceType(CommandSource source) const;

    RelayFanController &fan_;
    DualCurtainController &curtain_;
    BuzzerController &buzzer_;
    IRController &ir_;
    ControllerState state_;
};

#endif
