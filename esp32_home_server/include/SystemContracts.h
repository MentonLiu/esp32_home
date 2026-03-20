#ifndef SYSTEM_CONTRACTS_H
#define SYSTEM_CONTRACTS_H

#include <Arduino.h>
#include <functional>

enum class OperatingMode : uint8_t
{
    Cloud = 0,
    LocalAP = 1
};

enum class CommandSource : uint8_t
{
    LocalWeb = 0,
    CloudMqtt = 1,
    Automation = 2
};

enum class FanMode : uint8_t
{
    Off = 0,
    Low = 1,
    Medium = 2,
    High = 3
};

struct StandardSensorData
{
    float temperatureC = 0.0F;
    float humidityPercent = 0.0F;
    uint8_t lightPercent = 0;
    uint8_t mq2Percent = 0;
    String smokeLevel = "green";
    bool flameDetected = false;
    bool hasError = false;
    String errorMessage;
    unsigned long timestamp = 0;
};

struct ControllerState
{
    FanMode fanMode = FanMode::Off;
    uint8_t fanSpeedPercent = 0;
    uint8_t curtainAngle = 0;
    uint8_t lastCurtainPreset = 0;
    bool hasCurtainPreset = false;
    String lastIrCommand;
};

struct CommandResult
{
    bool accepted = false;
    bool stateChanged = false;
    String type = "info";
    String message;
};

using StatusReporter = std::function<void(const char *topic, const String &type, const String &message)>;

inline const char *modeToString(OperatingMode mode)
{
    return mode == OperatingMode::Cloud ? "cloud" : "local_ap";
}

inline const char *fanModeToString(FanMode mode)
{
    switch (mode)
    {
    case FanMode::Off:
        return "off";
    case FanMode::Low:
        return "low";
    case FanMode::Medium:
        return "medium";
    case FanMode::High:
        return "high";
    }

    return "off";
}

#endif
