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

    bool button1Pressed_ = false;
    bool button2Pressed_ = false;
    bool button3Pressed_ = false;
    bool button4Pressed_ = false;

    bool button1RawState_ = false;
    bool button2RawState_ = false;
    bool button3RawState_ = false;
    bool button4RawState_ = false;

    unsigned long button1ChangeMs_ = 0;
    unsigned long button2ChangeMs_ = 0;
    unsigned long button3ChangeMs_ = 0;
    unsigned long button4ChangeMs_ = 0;
};

#endif
