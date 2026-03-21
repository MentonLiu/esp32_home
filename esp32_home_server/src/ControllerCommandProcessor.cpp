// 文件说明：esp32_home_server/src/ControllerCommandProcessor.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "ControllerCommandProcessor.h"

#include <ArduinoJson.h>

ControllerCommandProcessor::ControllerCommandProcessor(RelayFanController &fan,
                                                       DualCurtainController &curtain,
                                                       BuzzerController &buzzer,
                                                       IRController &ir)
    : fan_(fan), curtain_(curtain), buzzer_(buzzer), ir_(ir)
{
}

void ControllerCommandProcessor::begin(Stream &irBridgeSerial)
{
    fan_.begin();
    curtain_.begin();
    buzzer_.begin();
    ir_.begin(irBridgeSerial);
    state_.fanPowerOn = false;
    refreshState();
}

CommandResult ControllerCommandProcessor::processCommandJson(const String &jsonText, CommandSource source)
{
    CommandResult result;
    result.type = sourceType(source);

    JsonDocument doc;
    const DeserializationError err = deserializeJson(doc, jsonText);
    if (err)
    {
        result.type = "error";
        result.message = String("invalid_control_json:") + err.c_str();
        return result;
    }

    const String device = doc["device"] | "";
    if (device.length() == 0)
    {
        result.type = "error";
        result.message = "device_missing";
        return result;
    }

    if (device == "fan")
    {
        bool handled = false;

        const String power = doc["power"] | "";
        if (power.length() > 0)
        {
            if (power == "on")
            {
                setFanPower(true);
            }
            else if (power == "off")
            {
                setFanPower(false);
            }
            else
            {
                result.type = "error";
                result.message = "fan_power_invalid";
                return result;
            }
            handled = true;
        }

        const String mode = doc["mode"] | "";
        if (mode.length() > 0)
        {
            if (!isFanPowerOn() && mode != "off")
            {
                result.type = "error";
                result.message = "fan_power_off";
                return result;
            }

            if (mode == "off")
            {
                setFanMode(FanMode::Off);
            }
            else if (mode == "low")
            {
                setFanMode(FanMode::Low);
            }
            else if (mode == "medium")
            {
                setFanMode(FanMode::Medium);
            }
            else if (mode == "high")
            {
                setFanMode(FanMode::High);
            }
            else
            {
                result.type = "error";
                result.message = "fan_mode_invalid";
                return result;
            }
            handled = true;
        }

        if (!doc["speedPercent"].isNull())
        {
            if (!isFanPowerOn())
            {
                result.type = "error";
                result.message = "fan_power_off";
                return result;
            }

            setFanSpeedPercent(doc["speedPercent"].as<uint8_t>());
            handled = true;
        }

        if (!handled)
        {
            result.type = "error";
            result.message = "fan_command_missing";
            return result;
        }

        result.accepted = true;
        result.stateChanged = true;
        result.message = "fan_updated";
        return result;
    }

    if (device == "curtain")
    {
        bool handled = false;

        if (!doc["angle"].isNull())
        {
            setCurtainAngle(doc["angle"].as<uint8_t>());
            handled = true;
        }

        if (!doc["preset"].isNull())
        {
            setCurtainPreset(doc["preset"].as<uint8_t>());
            handled = true;
        }

        if (!handled)
        {
            result.type = "error";
            result.message = "curtain_command_missing";
            return result;
        }

        result.accepted = true;
        result.stateChanged = true;
        result.message = "curtain_updated";
        return result;
    }

    if (device == "ir")
    {
        const String command = doc["command"] | "";
        if (command.length() == 0)
        {
            result.type = "error";
            result.message = "ir_command_missing";
            return result;
        }

        const bool ok = ir_.sendTextCommand(command);
        refreshState();
        result.accepted = ok;
        result.stateChanged = ok;
        result.type = ok ? sourceType(source) : "error";
        result.message = ok ? "ir_command_sent" : "ir_command_failed";
        return result;
    }

    result.type = "error";
    result.message = "unknown_device";
    return result;
}

void ControllerCommandProcessor::setFanPower(bool powerOn)
{
    state_.fanPowerOn = powerOn;
    if (!powerOn)
    {
        fan_.setSpeedPercent(0);
    }
    else if (fan_.speedPercent() == 0)
    {
        fan_.setMode(FanMode::Low);
    }
    refreshState();
}

bool ControllerCommandProcessor::isFanPowerOn() const
{
    return state_.fanPowerOn;
}

void ControllerCommandProcessor::setFanMode(FanMode mode)
{
    if (mode == FanMode::Off)
    {
        state_.fanPowerOn = false;
        fan_.setMode(FanMode::Off);
        refreshState();
        return;
    }

    state_.fanPowerOn = true;
    fan_.setMode(mode);
    refreshState();
}

void ControllerCommandProcessor::setFanSpeedPercent(uint8_t speedPercent)
{
    state_.fanPowerOn = true;
    fan_.setSpeedPercent(speedPercent);
    refreshState();
}

void ControllerCommandProcessor::setCurtainAngle(uint8_t angle)
{
    curtain_.setAngle(angle);
    state_.hasCurtainPreset = false;
    refreshState();
}

void ControllerCommandProcessor::setCurtainPreset(uint8_t preset)
{
    state_.lastCurtainPreset = constrain(preset, 0, 4);
    state_.hasCurtainPreset = true;
    curtain_.setPresetLevel(state_.lastCurtainPreset);
    refreshState();
}

void ControllerCommandProcessor::beep(uint16_t frequency, uint16_t durationMs)
{
    buzzer_.beep(frequency, durationMs);
}

void ControllerCommandProcessor::playFireAlarmPattern()
{
    buzzer_.patternShortShortLong();
}

bool ControllerCommandProcessor::pollIrBridgeMessage(String &payload)
{
    const IRDecodedSignal signal = ir_.receive();
    if (!signal.available)
    {
        return false;
    }

    payload = signal.payload;
    return true;
}

const ControllerState &ControllerCommandProcessor::state() const
{
    return state_;
}

void ControllerCommandProcessor::refreshState()
{
    state_.fanMode = fan_.mode();
    state_.fanSpeedPercent = fan_.speedPercent();
    state_.curtainAngle = curtain_.angle();
    state_.lastIrCommand = ir_.lastCommand();
}

const char *ControllerCommandProcessor::sourceType(CommandSource source) const
{
    switch (source)
    {
    case CommandSource::LocalWeb:
        return "local_command";
    case CommandSource::CloudMqtt:
        return "cloud_command";
    case CommandSource::Automation:
        return "automation";
    }

    return "command";
}
