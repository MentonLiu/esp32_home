// 文件说明：esp32_home_server/include/SensorDataProcessor.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef SENSOR_DATA_PROCESSOR_H
#define SENSOR_DATA_PROCESSOR_H

#include <Arduino.h>

#include "Sensor.h"
#include "SystemContracts.h"

class SensorDataProcessor
{
public:
    explicit SensorDataProcessor(SensorHub &sensorHub);

    void begin();
    bool loop();
    bool shouldPublish(unsigned long intervalMs = 500);

    const StandardSensorData &latest() const;
    String buildSensorJson() const;
    String buildStatusJson(OperatingMode mode, const String &ip, const ControllerState &controllerState) const;

private:
    void normalize(const SensorSnapshot &snapshot);

    SensorHub &sensorHub_;
    StandardSensorData latest_;
    bool hasFreshSample_ = false;
    unsigned long lastPublishMs_ = 0;
};

#endif
