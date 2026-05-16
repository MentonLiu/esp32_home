/**
 * 文件：esp32_home_server/include/Sensor.h
 * 功能说明：
 *   - 包裹 DHT11 温湿度传感器与模拟传感器（光照、烟雾、雨滴）
 *   - 处理 DHT 的最小采样间隔、读取失败不急读取、传感器默认值
 *   - 将模拟传感器的采样值映射为 0-100 百分比
 *   - 聚合传感器数据并实现自动采样、传感器程度低不例读取
 *   - 烟雾自动分级为 green/blue/yellow/red
 *
 * 核心类：
 *   - struct SensorSnapshot - 从物理传感器采样得到的原始快照
 *   - struct DhtReadResult - DHT 一次读取的结果
 *   - class DhtSensor - DHT11 温湿度传感器封装
 *   - class AnalogPercentSensor - 通用模拟传感器抄表
 *   - class SensorHub - 传感器聚约者
 *
 * 依赖：pins.h, SystemContracts.h, Logger.h, DHT 库
 * 被依赖于：SensorDataProcessor.h, AutomationEngine.h
 *
 * 设计详情：
 *   - DHT 控制采样间隔，至少 2.2秒间隔
 *   - DHT 读取失败自动优先使用旧值
 *   - 模拟传感器支持低体高反转（光照传感器一般低体有效）
 *   - 烟雾自动分级为 green/blue/yellow/red 四个步段
 */

#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <DHT.h>

/**
 * 从物理传感器采样得到的原始快照
 *
 * 功能：包含一次采样中所有传感器的数据、错误状态、时间戳
 */
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
    bool sensorStale = false;
    unsigned long sensorLastGoodTimestamp = 0;
    String sensorReadStatus = "booting";
    unsigned long timestamp = 0;
};

struct DhtReadResult
{
    float temperatureC = 0.0F;
    float humidityPercent = 0.0F;
    bool hasValidReading = false;
    bool usedCachedValue = false;
    String status = "booting";
    String error;
    unsigned long attemptedAt = 0;
    unsigned long lastGoodTimestamp = 0;
};

// 温湿度传感器封装，提供统一读取接口。
class DhtSensor
{
public:
    // sensorType 默认由上层传入（当前项目使用 DHT11）。
    DhtSensor(uint8_t pin, uint8_t sensorType);

    // 初始化 DHT 设备。
    void begin();
    // 读取温湿度，内部处理最小采样间隔、失败重试和旧值保留。
    DhtReadResult read();

    uint8_t pin() const;
    uint8_t sensorType() const;

private:
    DHT dht_;
    uint8_t pin_;
    uint8_t sensorType_;
    float lastTemperatureC_ = 0.0F;
    float lastHumidityPercent_ = 0.0F;
    bool hasLastValidReading_ = false;
    unsigned long lastAttemptMs_ = 0;
    unsigned long lastGoodMs_ = 0;
    unsigned long lastWarnLogMs_ = 0;
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
