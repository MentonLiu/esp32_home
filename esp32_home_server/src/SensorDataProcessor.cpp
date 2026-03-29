// 文件说明：esp32_home_server/src/SensorDataProcessor.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "SensorDataProcessor.h"

#include <ArduinoJson.h>

namespace
{
    constexpr uint8_t kRainOnThresholdPercent = 25;
    constexpr uint8_t kRainOffThresholdPercent = 15;
} // namespace

SensorDataProcessor::SensorDataProcessor(SensorHub &sensorHub) : sensorHub_(sensorHub) {}

void SensorDataProcessor::begin()
{
    // 初始化底层传感器集合。
    sensorHub_.begin();
}

bool SensorDataProcessor::loop()
{
    // 未到采样间隔时直接返回。
    if (!sensorHub_.poll())
    {
        return false;
    }

    // 产出新样本后做标准化并打新鲜标记。
    normalize(sensorHub_.latest());
    hasFreshSample_ = true;
    return true;
}

bool SensorDataProcessor::shouldPublish(unsigned long intervalMs)
{
    // 没有新样本就不触发发布。
    if (!hasFreshSample_)
    {
        return false;
    }

    // 发布频率节流。
    if (millis() - lastPublishMs_ < intervalMs)
    {
        return false;
    }

    lastPublishMs_ = millis();
    // 标记当前样本已被消费。
    hasFreshSample_ = false;
    return true;
}

const StandardSensorData &SensorDataProcessor::latest() const
{
    return latest_;
}

String SensorDataProcessor::buildSensorJson() const
{
    // 轻量传感器快照，用于上行遥测。
    JsonDocument doc;
    doc["sensorType"] = "home_snapshot";
    appendSensorFields(doc.to<JsonObject>());

    String payload;
    serializeJson(doc, payload);
    return payload;
}

String SensorDataProcessor::buildStatusJson(OperatingMode mode, const String &ip, const ControllerState &controllerState) const
{
    // 完整状态对象，兼容旧字段和嵌套字段两种读取方式。
    JsonDocument doc;
    doc["mode"] = modeToString(mode);
    doc["ip"] = ip;
    appendSensorFields(doc.to<JsonObject>());
    appendControllerFields(doc.to<JsonObject>(), controllerState);

    JsonObject sensor = doc["sensor"].to<JsonObject>();
    // 冗余一份 sensor 子对象，便于前端按分组读取。
    appendSensorFields(sensor);

    JsonObject controller = doc["controller"].to<JsonObject>();
    // 冗余一份 controller 子对象。
    appendControllerFields(controller, controllerState);
    if (controllerState.hasCurtainPreset)
    {
        controller["curtainPreset"] = controllerState.lastCurtainPreset;
    }

    String payload;
    serializeJson(doc, payload);
    return payload;
}

void SensorDataProcessor::normalize(const SensorSnapshot &snapshot)
{
    // 逐字段复制，保持含义明确，避免隐式转换副作用。
    latest_.temperatureC = snapshot.temperatureC;
    latest_.humidityPercent = snapshot.humidityPercent;
    latest_.lightPercent = snapshot.lightPercent;
    latest_.mq2Percent = snapshot.mq2Percent;
    latest_.rainPercent = snapshot.rainPercent;

    // 下雨判定使用滞回阈值，避免临界值抖动。
    if (rainingState_)
    {
        if (snapshot.rainPercent <= kRainOffThresholdPercent)
        {
            rainingState_ = false;
        }
    }
    else if (snapshot.rainPercent >= kRainOnThresholdPercent)
    {
        rainingState_ = true;
    }
    latest_.isRaining = rainingState_;

    latest_.smokeLevel = snapshot.smokeLevel;
    latest_.hasError = snapshot.hasError;
    latest_.errorMessage = snapshot.errorMessage;
    latest_.sensorStale = snapshot.sensorStale;
    latest_.sensorLastGoodTimestamp = snapshot.sensorLastGoodTimestamp;
    latest_.sensorReadStatus = snapshot.sensorReadStatus;
    latest_.timestamp = snapshot.timestamp;
}

void SensorDataProcessor::appendSensorFields(JsonObject target) const
{
    target["sensorTimestamp"] = latest_.timestamp;
    target["timestamp"] = latest_.timestamp;
    target["temperatureC"] = latest_.temperatureC;
    target["humidityPercent"] = latest_.humidityPercent;
    target["lightPercent"] = latest_.lightPercent;
    target["mq2Percent"] = latest_.mq2Percent;
    target["rainPercent"] = latest_.rainPercent;
    target["isRaining"] = latest_.isRaining;
    target["smokeLevel"] = latest_.smokeLevel;
    target["error"] = latest_.hasError;
    target["errorMessage"] = latest_.errorMessage;
    target["sensorStale"] = latest_.sensorStale;
    target["sensorLastGoodTimestamp"] = latest_.sensorLastGoodTimestamp;
    target["sensorReadStatus"] = latest_.sensorReadStatus;
}

void SensorDataProcessor::appendControllerFields(JsonObject target, const ControllerState &controllerState) const
{
    target["fanMode"] = fanModeToString(controllerState.fanMode);
    target["fanSpeedPercent"] = controllerState.fanSpeedPercent;
    target["fanOutputActive"] = controllerState.fanOutputActive;
    target["fanPwmDuty"] = controllerState.fanPwmDuty;
    target["curtainAngle"] = controllerState.curtainAngle;
}
