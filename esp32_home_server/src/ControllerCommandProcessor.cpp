// 文件说明：esp32_home_server/src/ControllerCommandProcessor.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "ControllerCommandProcessor.h"

#include <ArduinoJson.h>

ControllerCommandProcessor::ControllerCommandProcessor(RelayFanController &fan,
                                                       DualCurtainController &curtain,
                                                       BuzzerController &buzzer)
    : fan_(fan), curtain_(curtain), buzzer_(buzzer)
{
}

void ControllerCommandProcessor::begin()
{
    // 执行器初始化顺序：先风扇与窗帘，再蜂鸣器。
    fan_.begin();
    curtain_.begin();
    buzzer_.begin();
    // 读取一次真实状态，确保 state_ 和底层一致。
    refreshState();
}

void ControllerCommandProcessor::loop()
{
    buzzer_.loop();
    curtain_.loop();
}

CommandResult ControllerCommandProcessor::processCommandJson(const String &jsonText, CommandSource source)
{
    CommandResult result;
    // 默认把来源写入结果类型，便于上游区分来源。
    result.type = sourceType(source);

    JsonDocument doc;
    const DeserializationError err = deserializeJson(doc, jsonText);
    if (err)
    {
        // JSON 解析失败直接返回，避免误操作执行器。
        result.type = "error";
        result.message = String("invalid_control_json:") + err.c_str();
        return result;
    }

    const String device = doc["device"] | "";
    if (device.length() == 0)
    {
        // 设备字段缺失时不做任何控制。
        result.type = "error";
        result.message = "device_missing";
        return result;
    }

    if (device == "fan")
    {
        // fan 支持 mode/speedPercent 两种子命令组合。
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
                // 非法模式字符串。
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
            // fan 命令中未给任何可执行字段。
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
        // curtain 支持 angle 与 preset。
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
            // curtain 命令缺少具体动作。
            result.type = "error";
            result.message = "curtain_command_missing";
            return result;
        }

        // 非自动化来源的窗帘控制视为手动覆盖行为。
        if (source != CommandSource::Automation)
        {
            lastManualCurtainCommandMs_ = millis();
        }

        result.accepted = true;
        result.stateChanged = true;
        result.message = "curtain_updated";
        return result;
    }

    result.type = "error";
    result.message = "unknown_device";
    return result;
}

void ControllerCommandProcessor::setFanMode(FanMode mode)
{
    // Off 档即停机，其它档位按映射速度输出。
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
    // 手动角度会覆盖预设语义。
    curtain_.setAngle(angle);
    state_.hasCurtainPreset = false;
    refreshState();
}

void ControllerCommandProcessor::setCurtainPreset(uint8_t preset)
{
    // 预设编号限制在 0-4，防止数组越界。
    state_.lastCurtainPreset = constrain(preset, 0, 4);
    state_.hasCurtainPreset = true;
    curtain_.setPresetLevel(state_.lastCurtainPreset);
    refreshState();
}

void ControllerCommandProcessor::beep(uint16_t frequency, uint16_t durationMs)
{
    buzzer_.beep(frequency, durationMs);
}

const ControllerState &ControllerCommandProcessor::state() const
{
    return state_;
}

unsigned long ControllerCommandProcessor::lastManualCurtainCommandMs() const
{
    return lastManualCurtainCommandMs_;
}

void ControllerCommandProcessor::refreshState()
{
    // 从各控制器回读真实状态，避免“仅靠命令假设状态”。
    state_.fanMode = fan_.mode();
    state_.fanSpeedPercent = fan_.speedPercent();
    state_.curtainAngle = curtain_.angle();
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
