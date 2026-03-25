// 文件说明：esp32_home_server/include/Sensor.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <DHT.h>

// 从物理传感器采样得到的原始快照。
struct SensorSnapshot
{
    float temperatureC = 0.0F;
    float humidityPercent = 0.0F;
    uint8_t lightPercent = 0;
    uint8_t mq2Percent = 0;
    uint8_t rainPercent = 0;
    String smokeLevel = "green";
    bool hasError = false;
    String errorMessage;
    unsigned long timestamp = 0;
};

// 温湿度传感器封装，提供统一读取接口。
class DhtSensor
{
public:
    // sensorType 默认由上层传入（当前项目使用 DHT11）。
    DhtSensor(uint8_t pin, uint8_t sensorType);

    // 初始化 DHT 设备。
    void begin();
    // 读取温湿度；失败时返回 false 并写入 error。
    bool read(float &temperatureC, float &humidityPercent, String &error);

private:
    DHT dht_;
};

// 通用模数采样传感器，读数映射为百分比。
class AnalogPercentSensor
{
public:
    // inverted=true 时做百分比反转，适配“电压越低值越高”的传感器。
    AnalogPercentSensor(uint8_t pin, bool inverted = false, uint16_t adcMax = 4095);

    // 初始化 ADC 引脚。
    void begin() const;
    // 读取并映射到 0-100。
    uint8_t readPercent() const;

private:
    uint8_t pin_;
    bool inverted_;
    uint16_t adcMax_;
};

// 聚合所有传感器，并提供按间隔轮询能力。
class SensorHub
{
public:
    SensorHub(uint8_t dhtPin,
              uint8_t lightPin,
              uint8_t mq2Pin,
              uint8_t rainPin,
              uint8_t dhtType = DHT11);

    // 初始化全部传感器。
    void begin();
    // 按最小采样间隔轮询一次；返回 true 表示产出新样本。
    bool poll(unsigned long intervalMs = 500);
    // 获取最新采样快照。
    const SensorSnapshot &latest() const;

private:
    // 将烟雾传感百分比转换为颜色分级。
    String smokeLevelFromPercent(uint8_t percent) const;

    DhtSensor dht_;
    AnalogPercentSensor light_;
    AnalogPercentSensor mq2_;
    AnalogPercentSensor rain_;
    SensorSnapshot latest_;
    unsigned long lastSampleMs_ = 0;
};

#endif
