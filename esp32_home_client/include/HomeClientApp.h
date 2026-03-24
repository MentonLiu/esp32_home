// 文件说明：esp32_home_client/include/HomeClientApp.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef HOME_CLIENT_APP_H
#define HOME_CLIENT_APP_H

#include <Arduino.h>

#include "ClientContracts.h"
#include "ClientWiFiManager.h"
#include "InputManager.h"
#include "OutputManager.h"
#include "ServerApiClient.h"

class HomeClientApp
{
public:
    void begin();
    void loop();

private:
    void pollServerStatus();
    void handleInput();
    void handleEvent(const InputEvent &event);
    void flushPendingControl();
    ControlResponse sendControlPayload(const String &payload);
    void toggleFanPower();
    void syncDesiredStateFromServer();

    ClientWiFiManager wifiManager_;
    ServerApiClient apiClient_{wifiManager_};
    InputManager inputManager_;
    OutputManager outputManager_;

    ServerStatus serverStatus_;
    ClientControlMode controlMode_ = ClientControlMode::Curtain;
    unsigned long lastStatusPollMs_ = 0;
    unsigned long lastControlSendMs_ = 0;
    String lastMessage_ = "booting";

    int desiredCurtainAngle_ = 0;
    int desiredFanSpeed_ = 0;
    bool desiredFanPowerOn_ = false;
    bool pendingCurtainCommand_ = false;
    bool pendingFanCommand_ = false;
};

#endif
