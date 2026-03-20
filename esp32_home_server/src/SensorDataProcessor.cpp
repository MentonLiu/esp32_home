#include "SensorDataProcessor.h"

#include <ArduinoJson.h>

SensorDataProcessor::SensorDataProcessor(SensorHub &sensorHub) : sensorHub_(sensorHub) {}

void SensorDataProcessor::begin()
{
    sensorHub_.begin();
}

bool SensorDataProcessor::loop()
{
    // 轮询间隔未到时跳过本轮处理。
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
    // 仅在有新鲜样本时允许发布。
    if (!hasFreshSample_)
    {
        return false;
    }

    // 强制最小发布间隔。
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
    // 紧凑的上行传感器载荷。
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
    // 面向本地页面的状态载荷（含执行器状态）。
    JsonDocument doc;
    doc["mode"] = modeToString(mode);
    doc["ip"] = ip;
    doc["temperatureC"] = latest_.temperatureC;
    doc["humidityPercent"] = latest_.humidityPercent;
    doc["lightPercent"] = latest_.lightPercent;
    doc["mq2Percent"] = latest_.mq2Percent;
    doc["smokeLevel"] = latest_.smokeLevel;
    doc["flameDetected"] = latest_.flameDetected;
    doc["fanSpeedPercent"] = controllerState.fanSpeedPercent;
    doc["curtainAngle"] = controllerState.curtainAngle;
    doc["error"] = latest_.hasError;
    doc["errorMessage"] = latest_.errorMessage;

    String payload;
    serializeJson(doc, payload);
    return payload;
}

void SensorDataProcessor::normalize(const SensorSnapshot &snapshot)
{
    // 保持一份标准化副本供自动化与控制接口共享。
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
