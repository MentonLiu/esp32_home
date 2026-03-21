// 文件说明：esp32_home_server/src/SensorDataProcessor.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "SensorDataProcessor.h"

#include <ArduinoJson.h>

SensorDataProcessor::SensorDataProcessor(SensorHub &sensorHub) : sensorHub_(sensorHub) {}

void SensorDataProcessor::begin()
{
    sensorHub_.begin();
}

bool SensorDataProcessor::loop()
{
    if (!sensorHub_.poll())
    {
        return false;
    }

    normalize(sensorHub_.latest());
    hasFreshSample_ = true;
    return true;
}

bool SensorDataProcessor::shouldPublish(unsigned long intervalMs)
{
    if (!hasFreshSample_)
    {
        return false;
    }

    if (millis() - lastPublishMs_ < intervalMs)
    {
        return false;
    }

    lastPublishMs_ = millis();
    hasFreshSample_ = false;
    return true;
}

const StandardSensorData &SensorDataProcessor::latest() const
{
    return latest_;
}

String SensorDataProcessor::buildSensorJson() const
{
    JsonDocument doc;
    doc["sensorType"] = "home_snapshot";
    doc["timestamp"] = latest_.timestamp;
    doc["temperatureC"] = latest_.temperatureC;
    doc["humidityPercent"] = latest_.humidityPercent;
    doc["lightPercent"] = latest_.lightPercent;
    doc["mq2Percent"] = latest_.mq2Percent;
    doc["smokeLevel"] = latest_.smokeLevel;
    doc["flameDetected"] = latest_.flameDetected;
    doc["error"] = latest_.hasError;
    doc["errorMessage"] = latest_.errorMessage;

    String payload;
    serializeJson(doc, payload);
    return payload;
}

String SensorDataProcessor::buildStatusJson(OperatingMode mode, const String &ip, const ControllerState &controllerState) const
{
    JsonDocument doc;
    doc["mode"] = modeToString(mode);
    doc["ip"] = ip;
    doc["sensorTimestamp"] = latest_.timestamp;
    doc["temperatureC"] = latest_.temperatureC;
    doc["humidityPercent"] = latest_.humidityPercent;
    doc["lightPercent"] = latest_.lightPercent;
    doc["mq2Percent"] = latest_.mq2Percent;
    doc["smokeLevel"] = latest_.smokeLevel;
    doc["flameDetected"] = latest_.flameDetected;
    doc["fanMode"] = fanModeToString(controllerState.fanMode);
    doc["fanPowerOn"] = controllerState.fanPowerOn;
    doc["fanSpeedPercent"] = controllerState.fanSpeedPercent;
    doc["curtainAngle"] = controllerState.curtainAngle;
    doc["error"] = latest_.hasError;
    doc["errorMessage"] = latest_.errorMessage;

    JsonObject sensor = doc["sensor"].to<JsonObject>();
    sensor["temperatureC"] = latest_.temperatureC;
    sensor["humidityPercent"] = latest_.humidityPercent;
    sensor["lightPercent"] = latest_.lightPercent;
    sensor["mq2Percent"] = latest_.mq2Percent;
    sensor["smokeLevel"] = latest_.smokeLevel;
    sensor["flameDetected"] = latest_.flameDetected;
    sensor["timestamp"] = latest_.timestamp;
    sensor["error"] = latest_.hasError;
    sensor["errorMessage"] = latest_.errorMessage;

    JsonObject controller = doc["controller"].to<JsonObject>();
    controller["fanMode"] = fanModeToString(controllerState.fanMode);
    controller["fanPowerOn"] = controllerState.fanPowerOn;
    controller["fanSpeedPercent"] = controllerState.fanSpeedPercent;
    controller["curtainAngle"] = controllerState.curtainAngle;
    if (controllerState.hasCurtainPreset)
    {
        controller["curtainPreset"] = controllerState.lastCurtainPreset;
    }
    controller["lastIrCommand"] = controllerState.lastIrCommand;

    String payload;
    serializeJson(doc, payload);
    return payload;
}

void SensorDataProcessor::normalize(const SensorSnapshot &snapshot)
{
    latest_.temperatureC = snapshot.temperatureC;
    latest_.humidityPercent = snapshot.humidityPercent;
    latest_.lightPercent = snapshot.lightPercent;
    latest_.mq2Percent = snapshot.mq2Percent;
    latest_.smokeLevel = snapshot.smokeLevel;
    latest_.flameDetected = snapshot.flameDetected;
    latest_.hasError = snapshot.hasError;
    latest_.errorMessage = snapshot.errorMessage;
    latest_.timestamp = snapshot.timestamp;
}
