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
        const String mode = doc["mode"] | "";
        if (mode.length() > 0)
        {
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
        const String action = doc["action"] | "";
        bool ok = false;

        if (action == "send")
        {
            ok = ir_.sendProtocol(doc["protocol"] | "NEC",
                                  doc["address"] | 0,
                                  doc["command"] | 0,
                                  doc["repeats"] | 0);
        }
        else if (action == "send_json")
        {
            const String commandJson = doc["commandJson"] | "";
            ok = commandJson.length() > 0 && ir_.sendJson(commandJson);
        }
        else if (action.length() > 0)
        {
            String argsJson = "{}";
            if (!doc["args"].isNull())
            {
                argsJson = "";
                serializeJson(doc["args"], argsJson);
            }
            ok = ir_.sendAction(action, argsJson);
        }
        else
        {
            result.type = "error";
            result.message = "ir_action_missing";
            return result;
        }

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

void ControllerCommandProcessor::setFanMode(FanMode mode)
{
    fan_.setMode(mode);
    refreshState();
}

void ControllerCommandProcessor::setFanSpeedPercent(uint8_t speedPercent)
{
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
