// 文件说明：esp32_home_client/src/InputManager.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "InputManager.h"

#include "ClientConfig.h"
#include "ClientLog.h"

namespace
{
    constexpr unsigned long kDebounceMs = 35UL;
} // namespace

void InputManager::begin()
{
    pinMode(client_config::kButton1Pin, INPUT_PULLUP);
    pinMode(client_config::kButton2Pin, INPUT_PULLUP);
    pinMode(client_config::kButton3Pin, INPUT_PULLUP);
    pinMode(client_config::kButton4Pin, INPUT_PULLUP);
    CL_INFO("IN", "begin", "buttons=4 mode=input_pullup");
}

bool InputManager::nextEvent(InputEvent &event)
{
    event = {};

    if (readButtonPressed(client_config::kButton1Pin,
                          button1Pressed_,
                          button1RawState_,
                          button1ChangeMs_))
    {
        event.type = InputEventType::Button1;
        CL_INFO("IN", "button_press", "type=button1");
        return true;
    }
    if (readButtonPressed(client_config::kButton2Pin,
                          button2Pressed_,
                          button2RawState_,
                          button2ChangeMs_))
    {
        event.type = InputEventType::Button2;
        CL_INFO("IN", "button_press", "type=button2");
        return true;
    }
    if (readButtonPressed(client_config::kButton3Pin,
                          button3Pressed_,
                          button3RawState_,
                          button3ChangeMs_))
    {
        event.type = InputEventType::Button3;
        CL_INFO("IN", "button_press", "type=button3");
        return true;
    }
    if (readButtonPressed(client_config::kButton4Pin,
                          button4Pressed_,
                          button4RawState_,
                          button4ChangeMs_))
    {
        event.type = InputEventType::Button4;
        CL_INFO("IN", "button_press", "type=button4");
        return true;
    }

    return false;
}

bool InputManager::readButtonPressed(uint8_t pin, bool &stablePressed, bool &lastRawState, unsigned long &lastChangeMs)
{
    const bool rawPressed = digitalRead(pin) == LOW;
    if (rawPressed != lastRawState)
    {
        lastRawState = rawPressed;
        lastChangeMs = millis();
    }

    if (millis() - lastChangeMs < kDebounceMs)
    {
        return false;
    }

    if (rawPressed != stablePressed)
    {
        stablePressed = rawPressed;
        return stablePressed;
    }

    return false;
}
