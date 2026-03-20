#ifndef SENSOR_DATA_PROCESSOR_H
#define SENSOR_DATA_PROCESSOR_H

#include <Arduino.h>

#include "Sensor.h"
#include "SystemContracts.h"

// 将原始传感器快照转换为标准数据与结构化文本视图。
class SensorDataProcessor
{
public:
    explicit SensorDataProcessor(SensorHub &sensorHub);

    void begin();
    bool loop();
    // 发布节流门控：降低上行频率并强制最小间隔。
    bool shouldPublish(unsigned long intervalMs = 500);

    const StandardSensorData &latest() const;
    String buildSensorJson() const;
    String buildStatusJson(OperatingMode mode, const String &ip, const ControllerState &controllerState) const;

private:
    // 将原始快照复制并规范化到标准契约结构。
    void normalize(const SensorSnapshot &snapshot);

    SensorHub &sensorHub_;
    StandardSensorData latest_;
    bool hasFreshSample_ = false;
    unsigned long lastPublishMs_ = 0;
};

#endif
