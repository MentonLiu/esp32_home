#ifndef CONTROLLER_COMMAND_PROCESSOR_H
#define CONTROLLER_COMMAND_PROCESSOR_H

#include <Arduino.h>

#include "Controllerr.h"
#include "SystemContracts.h"

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

    bool pollIrBridgeMessage(String &payload);
    const ControllerState &state() const;

private:
    void refreshState();
    const char *sourceType(CommandSource source) const;

    RelayFanController &fan_;
    DualCurtainController &curtain_;
    BuzzerController &buzzer_;
    IRController &ir_;
    ControllerState state_;
};

#endif
