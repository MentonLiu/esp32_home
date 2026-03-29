// 文件说明：esp32_home_server/include/SensorDataProcessor.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef SENSOR_DATA_PROCESSOR_H
#define SENSOR_DATA_PROCESSOR_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include "Sensor.h"
#include "SystemContracts.h"

// 传感器数据处理器：
// 负责轮询 SensorHub、标准化数据结构、序列化状态 JSON。
class SensorDataProcessor
{
public:
    explicit SensorDataProcessor(SensorHub &sensorHub);

    // 初始化采集链路。
    void begin();
    // 驱动一次采样流程。
    bool loop();
    // 控制发布节流频率。
    bool shouldPublish(unsigned long intervalMs = 500);

    // 获取最新标准化数据。
    const StandardSensorData &latest() const;
    // 构建仅传感器数据 JSON（上行遥测用）。
    String buildSensorJson() const;
    // 构建完整状态 JSON（页面与调试接口用）。
    String buildStatusJson(OperatingMode mode, const String &ip, const ControllerState &controllerState) const;

private:
    // 将原始快照映射到统一结构。
    void normalize(const SensorSnapshot &snapshot);
    void appendSensorFields(JsonObject target) const;
    void appendControllerFields(JsonObject target, const ControllerState &controllerState) const;

    SensorHub &sensorHub_;
    StandardSensorData latest_;
    // 雨滴状态滞回，避免阈值边界抖动导致联动来回切换。
    bool rainingState_ = false;
    // 标记是否有未发布的新样本。
    bool hasFreshSample_ = false;
    // 上次发布时间戳，用于节流。
    unsigned long lastPublishMs_ = 0;
};

#endif
