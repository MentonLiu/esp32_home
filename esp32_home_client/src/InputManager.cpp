// 文件说明：esp32_home_client/src/InputManager.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "InputManager.h"

#include "ClientConfig.h"

namespace
{
    constexpr unsigned long kDebounceMs = 35UL;
    constexpr int8_t kEncoderTransitionTable[16] = {0, -1, 1, 0,
                                                    1, 0, 0, -1,
                                                    -1, 0, 0, 1,
                                                    0, 1, -1, 0};
} // namespace

void InputManager::begin()
{
    pinMode(client_config::kKey1Pin, INPUT_PULLUP);
    pinMode(client_config::kKey2Pin, INPUT_PULLUP);
    pinMode(client_config::kKey3Pin, INPUT_PULLUP);
    pinMode(client_config::kKey4Pin, INPUT_PULLUP);
    pinMode(client_config::kFanPowerButtonPin, INPUT_PULLUP);
    pinMode(client_config::kEncoderButtonPin, INPUT_PULLUP);
    pinMode(client_config::kEncoderPinA, INPUT);
    pinMode(client_config::kEncoderPinB, INPUT);

    encoderState_ = static_cast<uint8_t>((digitalRead(client_config::kEncoderPinA) << 1) |
                                         digitalRead(client_config::kEncoderPinB));
}

bool InputManager::nextEvent(InputEvent &event)
{
    event = {};

    if (readButtonPressed(client_config::kKey1Pin, key1Pressed_, key1RawState_, key1ChangeMs_))
    {
        event.type = InputEventType::Key1;
        return true;
    }
    if (readButtonPressed(client_config::kKey2Pin, key2Pressed_, key2RawState_, key2ChangeMs_))
    {
        event.type = InputEventType::Key2;
        return true;
    }
    if (readButtonPressed(client_config::kKey3Pin, key3Pressed_, key3RawState_, key3ChangeMs_))
    {
        event.type = InputEventType::Key3;
        return true;
    }
    if (readButtonPressed(client_config::kKey4Pin, key4Pressed_, key4RawState_, key4ChangeMs_))
    {
        event.type = InputEventType::Key4;
        return true;
    }
    if (readButtonPressed(client_config::kFanPowerButtonPin,
                          fanPowerButtonPressed_,
                          fanPowerButtonRawState_,
                          fanPowerButtonChangeMs_))
    {
        event.type = InputEventType::FanPowerButton;
        return true;
    }
    if (readButtonPressed(client_config::kEncoderButtonPin,
                          encoderButtonPressed_,
                          encoderButtonRawState_,
                          encoderButtonChangeMs_))
    {
        event.type = InputEventType::EncoderButton;
        return true;
    }

    updateEncoder();
    if (pendingEncoderSteps_ != 0)
    {
        event.type = InputEventType::EncoderAdjust;
        event.value = pendingEncoderSteps_;
        pendingEncoderSteps_ = 0;
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

void InputManager::updateEncoder()
{
    const uint8_t currentState = static_cast<uint8_t>((digitalRead(client_config::kEncoderPinA) << 1) |
                                                      digitalRead(client_config::kEncoderPinB));
    if (currentState == encoderState_)
    {
        return;
    }

    encoderState_ = static_cast<uint8_t>(((encoderState_ << 2) | currentState) & 0x0F);
    encoderAccumulator_ += kEncoderTransitionTable[encoderState_];

    if (encoderAccumulator_ >= 4)
    {
        ++pendingEncoderSteps_;
        encoderAccumulator_ = 0;
    }
    else if (encoderAccumulator_ <= -4)
    {
        --pendingEncoderSteps_;
        encoderAccumulator_ = 0;
    }
}
