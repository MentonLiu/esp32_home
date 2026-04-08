// 文件说明：esp32_home_client/include/ClientContracts.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef CLIENT_CONTRACTS_H
#define CLIENT_CONTRACTS_H

#include <Arduino.h>

enum class ClientControlMode : uint8_t
{
    // 窗帘角度控制。
    Curtain = 0,
    // 风扇模式/转速控制。
    Fan = 1
};

enum class InputEventType : uint8_t
{
    // 无待处理事件。
    None = 0,
    Button1,
    Button2,
    Button3,
    Button4
};

struct InputEvent
{
    // 触发该事件的物理输入类型。
    InputEventType type = InputEventType::None;
    // 预留扩展字段。
    int value = 0;
};

struct ServerStatus
{
    // 最近一次服务端状态拉取是否成功。
    bool online = false;
    String serverBaseUrl;
    String mode = "offline";
    String ip;
    float temperatureC = 0.0F;
    float humidityPercent = 0.0F;
    uint8_t lightPercent = 0;
    uint8_t mq2Percent = 0;
    String smokeLevel = "green";
    bool flameDetected = false;
    String fanMode = "off";
    uint8_t fanSpeedPercent = 0;
    uint8_t curtainAngle = 0;
    bool error = false;
    String errorMessage;
    unsigned long sensorTimestamp = 0;
};

struct ControlResponse
{
    // 命令是否被服务端接受。
    bool ok = false;
    // 远端设备状态是否发生变化。
    bool stateChanged = false;
    int httpCode = 0;
    String type;
    String message;
};

inline const char *controlModeName(ClientControlMode mode)
{
    // 日志与界面共用的模式文本映射。
    return mode == ClientControlMode::Curtain ? "CURTAIN" : "FAN";
}

#endif
