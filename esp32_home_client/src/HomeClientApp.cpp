// 文件说明：esp32_home_client/src/HomeClientApp.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "HomeClientApp.h"

#include "ClientConfig.h"

void HomeClientApp::begin()
{
    Serial.begin(115200);
    outputManager_.begin();
    inputManager_.begin();
    wifiManager_.begin();
    apiClient_.begin();

    serverStatus_.smokeLevel = "green";
    lastMessage_ = "client_ready";
}

void HomeClientApp::loop()
{
    wifiManager_.loop();
    pollServerStatus();
    handleInput();
    flushPendingControl();

    outputManager_.render(wifiManager_,
                          serverStatus_,
                          controlMode_,
                          lastMessage_,
                          apiClient_.isServerReachable());
}

void HomeClientApp::pollServerStatus()
{
    if (millis() - lastStatusPollMs_ < client_config::kStatusPollIntervalMs)
    {
        return;
    }
    lastStatusPollMs_ = millis();

    ServerStatus status;
    if (!apiClient_.fetchStatus(status))
    {
        serverStatus_.online = false;
        if (!wifiManager_.isConnected())
        {
            lastMessage_ = "wifi_disconnected";
        }
        else
        {
            lastMessage_ = "server_searching";
        }
        return;
    }

    serverStatus_ = status;
    syncDesiredStateFromServer();
}

void HomeClientApp::handleInput()
{
    InputEvent event;
    while (inputManager_.nextEvent(event))
    {
        handleEvent(event);
    }
}

void HomeClientApp::handleEvent(const InputEvent &event)
{
    switch (event.type)
    {
    case InputEventType::FanPowerButton:
        toggleFanPower();
        break;
    case InputEventType::EncoderButton:
        controlMode_ = controlMode_ == ClientControlMode::Curtain ? ClientControlMode::Fan : ClientControlMode::Curtain;
        lastMessage_ = String("mode_") + controlModeName(controlMode_);
        break;
    case InputEventType::EncoderAdjust:
        if (controlMode_ == ClientControlMode::Curtain)
        {
            desiredCurtainAngle_ = constrain(desiredCurtainAngle_ + event.value * client_config::kCurtainStepDegrees, 0, 180);
            pendingCurtainCommand_ = true;
            lastMessage_ = String("curtain_") + desiredCurtainAngle_;
        }
        else
        {
            if (!desiredFanPowerOn_)
            {
                lastMessage_ = "fan_power_off";
                break;
            }
            desiredFanSpeed_ = constrain(desiredFanSpeed_ + event.value * client_config::kFanStepPercent, 0, 100);
            pendingFanCommand_ = true;
            lastMessage_ = String("fan_") + desiredFanSpeed_;
        }
        break;
    case InputEventType::None:
        break;
    }
}

void HomeClientApp::flushPendingControl()
{
    if (millis() - lastControlSendMs_ < client_config::kControlCooldownMs)
    {
        return;
    }

    if (pendingCurtainCommand_)
    {
        pendingCurtainCommand_ = false;
        lastControlSendMs_ = millis();
        const String payload = String("{\"device\":\"curtain\",\"angle\":") + desiredCurtainAngle_ + "}";
        const ControlResponse response = sendControlPayload(payload);
        lastMessage_ = response.ok ? String("curtain_ok_") + desiredCurtainAngle_ : String("curtain_fail_") + response.message;
        return;
    }

    if (pendingFanCommand_)
    {
        pendingFanCommand_ = false;
        lastControlSendMs_ = millis();
        const String payload = String("{\"device\":\"fan\",\"speedPercent\":") + desiredFanSpeed_ + "}";
        const ControlResponse response = sendControlPayload(payload);
        lastMessage_ = response.ok ? String("fan_ok_") + desiredFanSpeed_ : String("fan_fail_") + response.message;
    }
}

ControlResponse HomeClientApp::sendControlPayload(const String &payload)
{
    lastControlSendMs_ = millis();
    return apiClient_.sendCommand(payload);
}

void HomeClientApp::toggleFanPower()
{
    desiredFanPowerOn_ = !desiredFanPowerOn_;
    if (!desiredFanPowerOn_)
    {
        desiredFanSpeed_ = 0;
        pendingFanCommand_ = false;
    }

    const String powerState = desiredFanPowerOn_ ? "on" : "off";
    const String payload = String("{\"device\":\"fan\",\"power\":\"") + powerState + "\"}";
    const ControlResponse response = sendControlPayload(payload);
    if (!response.ok)
    {
        desiredFanPowerOn_ = serverStatus_.fanPowerOn;
        lastMessage_ = String("fan_power_fail_") + response.message;
        return;
    }

    lastMessage_ = desiredFanPowerOn_ ? "fan_power_on" : "fan_power_off";
}

void HomeClientApp::syncDesiredStateFromServer()
{
    if (!pendingCurtainCommand_)
    {
        desiredCurtainAngle_ = serverStatus_.curtainAngle;
    }

    if (!pendingFanCommand_)
    {
        desiredFanSpeed_ = serverStatus_.fanSpeedPercent;
    }

    desiredFanPowerOn_ = serverStatus_.fanPowerOn;
}
