/**
 * 文件：esp32_home_server/src/SensorDataProcessor.cpp
 * 功能说明：
 *   - 实现传感器数据处理逻辑：轮询、归一化、JSON 序列化
 *   - 处理雨滴状态滞回（阈值：开启 25%，关闭 15%）
 *   - 提供两类 JSON 输出：传感器遥测和完整系统状态
 *   - 实现发布节流控制（默认 500ms）
 *
 * 核心实现：
 *   - SensorDataProcessor::loop() - 驱动采样和数据更新
 *   - SensorDataProcessor::normalize() - 映射原始数据到统一格式
 *   - SensorDataProcessor::buildSensorJson() - 传感器遥测 JSON
 *   - SensorDataProcessor::buildStatusJson() - 完整状态 JSON
 *
 * 依赖：SensorDataProcessor.h, Sensor.h, ArduinoJson 库
 * 被依赖于：ConnectivityManager.cpp, LocalProcessingProgram.cpp
 *
 * 设计细节：
 *   - 雨滴滞回防止频繁切换（开启 25%，关闭 15%）
 *   - 烟雾自动分级：green (0-75%)、blue/yellow (75-90%)、red (>90%)
 *   - 发布节流避免网络消息洪泛和 MQTT 带宽浪费
 */

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
    JsonObject root = doc.to<JsonObject>();
    root["sensorType"] = "home_snapshot";
    appendSensorFields(root);

    String payload;
    serializeJson(doc, payload);
    return payload;
}

String SensorDataProcessor::buildStatusJson(OperatingMode mode, const String &ip, const ControllerState &controllerState) const
{
    // 完整状态对象，兼容旧字段和嵌套字段两种读取方式。
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["mode"] = modeToString(mode);
    root["ip"] = ip;
    appendSensorFields(root);
    appendControllerFields(root, controllerState);

    JsonObject sensor = root["sensor"].to<JsonObject>();
    // 冗余一份 sensor 子对象，便于前端按分组读取。
    appendSensorFields(sensor);

    JsonObject controller = root["controller"].to<JsonObject>();
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
