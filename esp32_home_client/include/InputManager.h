// 文件说明：esp32_home_client/include/InputManager.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>

#include "ClientContracts.h"

class InputManager
{
public:
    void begin();
    bool nextEvent(InputEvent &event);

private:
    bool readButtonPressed(uint8_t pin, bool &stablePressed, bool &lastRawState, unsigned long &lastChangeMs);
    void updateEncoder();

    bool key1Pressed_ = false;
    bool key2Pressed_ = false;
    bool key3Pressed_ = false;
    bool key4Pressed_ = false;
    bool fanPowerButtonPressed_ = false;
    bool encoderButtonPressed_ = false;

    bool key1RawState_ = false;
    bool key2RawState_ = false;
    bool key3RawState_ = false;
    bool key4RawState_ = false;
    bool fanPowerButtonRawState_ = false;
    bool encoderButtonRawState_ = false;

    unsigned long key1ChangeMs_ = 0;
    unsigned long key2ChangeMs_ = 0;
    unsigned long key3ChangeMs_ = 0;
    unsigned long key4ChangeMs_ = 0;
    unsigned long fanPowerButtonChangeMs_ = 0;
    unsigned long encoderButtonChangeMs_ = 0;

    uint8_t encoderState_ = 0;
    int encoderAccumulator_ = 0;
    int pendingEncoderSteps_ = 0;
};

#endif
