// 文件说明：esp32_home_client/src/HomeClientApp.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "HomeClientApp.h"

#include "ClientConfig.h"

namespace
{
    constexpr uint8_t kCurtainAngles[] = {0, 45, 90, 135, 180};
    constexpr const char *kFanModes[] = {"off", "low", "medium", "high"};
} // namespace

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
    case InputEventType::Button1:
        advanceCurtainState();
        break;
    case InputEventType::Button2:
        advanceFanState();
        break;
    case InputEventType::Button3:
        lastMessage_ = "button3_reserved";
        break;
    case InputEventType::Button4:
        lastMessage_ = "button4_reserved";
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
        const String payload = String("{\"device\":\"fan\",\"mode\":\"") + desiredFanMode_ + "\"}";
        const ControlResponse response = sendControlPayload(payload);
        lastMessage_ = response.ok ? String("fan_ok_") + desiredFanMode_ : String("fan_fail_") + response.message;
    }
}

ControlResponse HomeClientApp::sendControlPayload(const String &payload)
{
    lastControlSendMs_ = millis();
    return apiClient_.sendCommand(payload);
}

void HomeClientApp::advanceCurtainState()
{
    const int nextIndex = (curtainStateIndexFromAngle(static_cast<uint8_t>(desiredCurtainAngle_)) + 1) % 5;
    desiredCurtainAngle_ = kCurtainAngles[nextIndex];
    pendingCurtainCommand_ = true;
    lastMessage_ = String("curtain_") + desiredCurtainAngle_;
}

void HomeClientApp::advanceFanState()
{
    const int nextIndex = (fanStateIndexFromMode(desiredFanMode_, serverStatus_.fanSpeedPercent) + 1) % 4;
    desiredFanMode_ = kFanModes[nextIndex];
    pendingFanCommand_ = true;
    lastMessage_ = String("fan_") + desiredFanMode_;
}

void HomeClientApp::syncDesiredStateFromServer()
{
    if (!pendingCurtainCommand_)
    {
        desiredCurtainAngle_ = serverStatus_.curtainAngle;
    }

    if (!pendingFanCommand_)
    {
        desiredFanMode_ = serverStatus_.fanMode;
    }
}

int HomeClientApp::curtainStateIndexFromAngle(uint8_t angle) const
{
    if (angle < 23)
    {
        return 0;
    }
    if (angle < 68)
    {
        return 1;
    }
    if (angle < 113)
    {
        return 2;
    }
    if (angle < 158)
    {
        return 3;
    }
    return 4;
}

int HomeClientApp::fanStateIndexFromMode(const String &fanMode, uint8_t fanSpeedPercent) const
{
    if (fanMode == "low")
    {
        return 1;
    }
    if (fanMode == "medium")
    {
        return 2;
    }
    if (fanMode == "high")
    {
        return 3;
    }
    if (fanSpeedPercent >= 80)
    {
        return 3;
    }
    if (fanSpeedPercent >= 50)
    {
        return 2;
    }
    if (fanSpeedPercent > 0)
    {
        return 1;
    }
    return 0;
}
