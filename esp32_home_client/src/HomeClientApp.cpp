// 文件说明：esp32_home_client/src/HomeClientApp.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "HomeClientApp.h"

#include "ClientConfig.h"
#include "ClientLog.h"

namespace
{
    // 按键循环切换时使用的预设状态序列。
    constexpr uint8_t kCurtainAngles[] = {0, 45, 90, 135, 180};
    constexpr const char *kFanModes[] = {"off", "low", "medium", "high"};
} // namespace

void HomeClientApp::begin()
{
    // 优先启动串口，便于早期启动诊断。
    const unsigned long beginStartMs = millis();
    Serial.begin(115200);

    CL_INFO("APP", "begin", "phase=start");

    unsigned long phaseStartMs = millis();
    CL_INFO("APP", "init_phase", "phase=output_begin_start");
    // 先初始化输出，再初始化输入/网络，以便尽早显示启动信息。
    outputManager_.begin();
    CL_INFOF("APP", "init_phase", "phase=output_begin_done dur_ms=%lu", millis() - phaseStartMs);

    phaseStartMs = millis();
    CL_INFO("APP", "init_phase", "phase=input_begin_start");
    inputManager_.begin();
    CL_INFOF("APP", "init_phase", "phase=input_begin_done dur_ms=%lu", millis() - phaseStartMs);

    phaseStartMs = millis();
    CL_INFO("APP", "init_phase", "phase=wifi_begin_start");
    wifiManager_.begin();
    CL_INFOF("APP", "init_phase", "phase=wifi_begin_done dur_ms=%lu", millis() - phaseStartMs);

    phaseStartMs = millis();
    CL_INFO("APP", "init_phase", "phase=api_begin_start");
    apiClient_.begin();
    CL_INFOF("APP", "init_phase", "phase=api_begin_done dur_ms=%lu", millis() - phaseStartMs);

    serverStatus_.smokeLevel = "green";
    lastMessage_ = "client_ready";
    CL_INFOF("APP", "ready", "message=%s total_ms=%lu", lastMessage_.c_str(), millis() - beginStartMs);
}

void HomeClientApp::loop()
{
    // 主循环顺序：维持链路、刷新状态、处理命令、执行渲染。
    wifiManager_.loop();
    pollServerStatus();
    handleInput();
    flushPendingControl();

    outputManager_.render(wifiManager_,
                          serverStatus_,
                          lastMessage_,
                          apiClient_.isServerReachable());

    static unsigned long lastLoopLogMs = 0;
    if (client_log::allowPeriodic(lastLoopLogMs, client_config::kDiagnosticsPeriodicLogMs))
    {
        CL_INFOF("APP", "loop_summary",
                 "wifi=%d server=%d pending_curtain=%d pending_fan=%d",
                 wifiManager_.isConnected() ? 1 : 0,
                 apiClient_.isServerReachable() ? 1 : 0,
                 pendingCurtainCommand_ ? 1 : 0,
                 pendingFanCommand_ ? 1 : 0);
    }
}

void HomeClientApp::pollServerStatus()
{
    // 按固定节奏轮询，避免对服务端产生过高压力。
    if (millis() - lastStatusPollMs_ < client_config::kStatusPollIntervalMs)
    {
        return;
    }
    lastStatusPollMs_ = millis();
    CL_INFO("API", "poll_start", "path=home_loop");

    ServerStatus status;
    const unsigned long fetchStartMs = millis();
    if (!apiClient_.fetchStatus(status))
    {
        // 区分网络断开与服务端搜索中，保证 UI 提示语义明确。
        serverStatus_.online = false;
        if (!wifiManager_.isConnected())
        {
            lastMessage_ = "wifi_disconnected";
        }
        else
        {
            lastMessage_ = "server_searching";
        }
        CL_WARNF("API", "poll_fail", "dur_ms=%lu message=%s", millis() - fetchStartMs, lastMessage_.c_str());
        return;
    }

    serverStatus_ = status;
    syncDesiredStateFromServer();
    CL_INFOF("API", "poll_ok", "dur_ms=%lu mode=%s smoke=%s", millis() - fetchStartMs, serverStatus_.mode.c_str(), serverStatus_.smokeLevel.c_str());
}

void HomeClientApp::handleInput()
{
    // 清空事件队列，避免快速连按被丢失。
    InputEvent event;
    while (inputManager_.nextEvent(event))
    {
        handleEvent(event);
    }
}

void HomeClientApp::handleEvent(const InputEvent &event)
{
    // 将物理按键动作映射为业务状态切换。
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
    // 短冷却时间可避免误触导致的命令突发。
    if (millis() - lastControlSendMs_ < client_config::kControlCooldownMs)
    {
        return;
    }

    if (pendingCurtainCommand_)
    {
        // 窗帘与风扇命令串行发送，保证意图顺序。
        pendingCurtainCommand_ = false;
        lastControlSendMs_ = millis();
        const String payload = String("{\"device\":\"curtain\",\"angle\":") + desiredCurtainAngle_ + "}";
        CL_INFOF("CTRL", "send", "device=curtain angle=%d", desiredCurtainAngle_);
        const ControlResponse response = sendControlPayload(payload);
        lastMessage_ = response.ok ? String("curtain_ok_") + desiredCurtainAngle_ : String("curtain_fail_") + response.message;
        CL_INFOF("CTRL", "result", "device=curtain ok=%d http=%d msg=%s", response.ok ? 1 : 0, response.httpCode, response.message.c_str());
        return;
    }

    if (pendingFanCommand_)
    {
        pendingFanCommand_ = false;
        lastControlSendMs_ = millis();
        const String payload = String("{\"device\":\"fan\",\"mode\":\"") + desiredFanMode_ + "\"}";
        CL_INFOF("CTRL", "send", "device=fan mode=%s", desiredFanMode_.c_str());
        const ControlResponse response = sendControlPayload(payload);
        lastMessage_ = response.ok ? String("fan_ok_") + desiredFanMode_ : String("fan_fail_") + response.message;
        CL_INFOF("CTRL", "result", "device=fan ok=%d http=%d msg=%s", response.ok ? 1 : 0, response.httpCode, response.message.c_str());
    }
}

ControlResponse HomeClientApp::sendControlPayload(const String &payload)
{
    // 统一发送控制负载并记录耗时。
    lastControlSendMs_ = millis();
    const unsigned long startMs = millis();
    ControlResponse response = apiClient_.sendCommand(payload);
    CL_INFOF("CTRL", "send_done", "dur_ms=%lu ok=%d http=%d", millis() - startMs, response.ok ? 1 : 0, response.httpCode);
    return response;
}

void HomeClientApp::advanceCurtainState()
{
    // 在 5 档角度环中循环。
    const int nextIndex = (curtainStateIndexFromAngle(static_cast<uint8_t>(desiredCurtainAngle_)) + 1) % 5;
    desiredCurtainAngle_ = kCurtainAngles[nextIndex];
    pendingCurtainCommand_ = true;
    lastMessage_ = String("curtain_") + desiredCurtainAngle_;
    CL_INFOF("IN", "curtain_cycle", "target_angle=%d", desiredCurtainAngle_);
}

void HomeClientApp::advanceFanState()
{
    // 在 4 档风扇模式环中循环。
    const int nextIndex = (fanStateIndexFromMode(desiredFanMode_, serverStatus_.fanSpeedPercent) + 1) % 4;
    desiredFanMode_ = kFanModes[nextIndex];
    pendingFanCommand_ = true;
    lastMessage_ = String("fan_") + desiredFanMode_;
    CL_INFOF("IN", "fan_cycle", "target_mode=%s", desiredFanMode_.c_str());
}

void HomeClientApp::syncDesiredStateFromServer()
{
    // 有本地下发中的命令时不覆盖，仅在无待发命令时同步。
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
    // 将连续角度映射到最近的预设档位。
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
    // 优先使用服务端明确模式，否则按转速推断模式。
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
