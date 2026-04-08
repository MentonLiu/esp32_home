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
    // 按固定顺序初始化所有子系统。
    void begin();
    // 主循环：网络、输入、控制、渲染。
    void loop();

private:
    // 拉取服务端最新状态快照。
    void pollServerStatus();
    // 消费 InputManager 中排队的输入事件。
    void handleInput();
    // 将单个输入事件路由到对应动作。
    void handleEvent(const InputEvent &event);
    // 在冷却时间约束下发送待下发命令。
    void flushPendingControl();
    // 统一封装命令发送与耗时日志。
    ControlResponse sendControlPayload(const String &payload);
    // 循环切换窗帘预设角度。
    void advanceCurtainState();
    // 循环切换风扇模式。
    void advanceFanState();
    // 根据服务端状态同步本地目标状态。
    void syncDesiredStateFromServer();
    // 将角度映射到最近的预设档位索引。
    int curtainStateIndexFromAngle(uint8_t angle) const;
    // 将风扇模式/转速映射到本地模式索引。
    int fanStateIndexFromMode(const String &fanMode, uint8_t fanSpeedPercent) const;

    ClientWiFiManager wifiManager_;
    ServerApiClient apiClient_{wifiManager_};
    InputManager inputManager_;
    OutputManager outputManager_;

    // 服务端返回的最新状态。
    ServerStatus serverStatus_;
    unsigned long lastStatusPollMs_ = 0;
    unsigned long lastControlSendMs_ = 0;
    String lastMessage_ = "booting";

    int desiredCurtainAngle_ = 0;
    String desiredFanMode_ = "off";
    bool pendingCurtainCommand_ = false;
    bool pendingFanCommand_ = false;
};

#endif
