// 文件说明：esp32_home_client/include/ClientContracts.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef CLIENT_CONTRACTS_H
#define CLIENT_CONTRACTS_H

#include <Arduino.h>

enum class ClientControlMode : uint8_t
{
    Curtain = 0,
    Fan = 1
};

enum class InputEventType : uint8_t
{
    None = 0,
    Button1,
    Button2,
    Button3,
    Button4
};

struct InputEvent
{
    InputEventType type = InputEventType::None;
    int value = 0;
};

struct ServerStatus
{
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
    bool ok = false;
    bool stateChanged = false;
    int httpCode = 0;
    String type;
    String message;
};

inline const char *controlModeName(ClientControlMode mode)
{
    return mode == ClientControlMode::Curtain ? "CURTAIN" : "FAN";
}

#endif
