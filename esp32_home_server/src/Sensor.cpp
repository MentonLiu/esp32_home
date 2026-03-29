// 文件说明：esp32_home_server/src/Sensor.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "Sensor.h"

#include <math.h>
#include "Logger.h"

namespace
{
    constexpr unsigned long kDhtMinReadIntervalMs = 2200;
    constexpr uint8_t kDhtRetryCount = 2;
    constexpr unsigned long kDhtRetryDelayMs = 30;
    constexpr unsigned long kDhtWarnIntervalMs = 10000;
} // namespace

DhtSensor::DhtSensor(uint8_t pin, uint8_t sensorType)
    : dht_(pin, sensorType), pin_(pin), sensorType_(sensorType) {}

void DhtSensor::begin()
{
    // DHT 库内部会完成引脚与时序初始化。
    dht_.begin();
    LOG_INFO("DHT", "DHT 传感器初始化完成: pin=%u type=%s", pin_, sensorType_ == DHT11 ? "DHT11" : "UNKNOWN");
}

DhtReadResult DhtSensor::read()
{
    DhtReadResult result;
    result.attemptedAt = millis();
    result.lastGoodTimestamp = lastGoodMs_;

    if (hasLastValidReading_ && result.attemptedAt - lastAttemptMs_ < kDhtMinReadIntervalMs)
    {
        result.temperatureC = lastTemperatureC_;
        result.humidityPercent = lastHumidityPercent_;
        result.hasValidReading = true;
        result.usedCachedValue = true;
        result.status = "cached";
        return result;
    }

    lastAttemptMs_ = result.attemptedAt;
    for (uint8_t attempt = 0; attempt < kDhtRetryCount; ++attempt)
    {
        const float temperatureC = dht_.readTemperature();
        const float humidityPercent = dht_.readHumidity();
        if (!isnan(temperatureC) && !isnan(humidityPercent))
        {
            lastTemperatureC_ = temperatureC;
            lastHumidityPercent_ = humidityPercent;
            lastGoodMs_ = millis();
            hasLastValidReading_ = true;

            result.temperatureC = temperatureC;
            result.humidityPercent = humidityPercent;
            result.hasValidReading = true;
            result.usedCachedValue = false;
            result.status = "ok";
            result.lastGoodTimestamp = lastGoodMs_;
            return result;
        }

        if (attempt + 1 < kDhtRetryCount)
        {
            delay(kDhtRetryDelayMs);
        }
    }

    result.error = "dht_read_failed";
    if (hasLastValidReading_)
    {
        result.temperatureC = lastTemperatureC_;
        result.humidityPercent = lastHumidityPercent_;
        result.hasValidReading = true;
        result.usedCachedValue = true;
        result.status = "recovered_with_last_value";
        result.lastGoodTimestamp = lastGoodMs_;
    }
    else
    {
        result.status = "no_valid_data";
    }

    if (result.attemptedAt - lastWarnLogMs_ >= kDhtWarnIntervalMs)
    {
        LOG_WARN("DHT",
                 "读取失败: pin=%u type=%s status=%s，检查接线/供电/上拉电阻",
                 pin_,
                 sensorType_ == DHT11 ? "DHT11" : "UNKNOWN",
                 result.status.c_str());
        lastWarnLogMs_ = result.attemptedAt;
    }

    return result;
}

uint8_t DhtSensor::pin() const
{
    return pin_;
}

uint8_t DhtSensor::sensorType() const
{
    return sensorType_;
}

// 模拟量传感器通过 ADC 读取并映射为百分比，支持反转与自定义最大值。
AnalogPercentSensor::AnalogPercentSensor(uint8_t pin, bool inverted, uint16_t adcMax)
    : pin_(pin), inverted_(inverted), adcMax_(adcMax) {}

void AnalogPercentSensor::begin() const
{
    pinMode(pin_, INPUT);
}

uint8_t AnalogPercentSensor::readPercent() const
{
    // 将模数采样读数归一化为 0-100 百分比。
    int raw = analogRead(pin_);
    raw = constrain(raw, 0, adcMax_);

    int percent = map(raw, 0, adcMax_, 0, 100);
    if (inverted_)
    {
        percent = 100 - percent;
    }

    return static_cast<uint8_t>(constrain(percent, 0, 100));
}

SensorHub::SensorHub(uint8_t dhtPin,
                     uint8_t lightPin,
                     uint8_t mq2Pin,
                     uint8_t rainPin,
                     uint8_t dhtType)
    : dht_(dhtPin, dhtType),
      light_(lightPin, true),
      mq2_(mq2Pin, false),
      rain_(rainPin, true)
{
    // 构造阶段仅绑定引脚与参数，不做硬件访问。
}

void SensorHub::begin()
{
    // 一次性初始化全部硬件通道。
    LOG_INFO("SENSOR", "初始化传感器集群...");
    dht_.begin();
    LOG_DEBUG("SENSOR", "DHT初始化完成");
    light_.begin();
    LOG_DEBUG("SENSOR", "光照传感器初始化完成");
    mq2_.begin();
    LOG_DEBUG("SENSOR", "MQ2烟雾传感器初始化完成");
    rain_.begin();
    LOG_DEBUG("SENSOR", "雨滴传感器初始化完成");
    LOG_INFO("SENSOR", "所有传感器已就绪");
}

bool SensorHub::poll(unsigned long intervalMs)
{
    // 维持固定最小采样间隔，降低传感器抖动与开销。
    const unsigned long now = millis();
    if (now - lastSampleMs_ < intervalMs)
    {
        return false;
    }

    lastSampleMs_ = now;
    // 本次采样前先清空错误状态。
    latest_.timestamp = now;
    latest_.hasError = false;
    latest_.errorMessage = "";

    const DhtReadResult dhtResult = dht_.read();
    latest_.sensorReadStatus = dhtResult.status;
    latest_.sensorLastGoodTimestamp = dhtResult.lastGoodTimestamp;

    if (dhtResult.hasValidReading)
    {
        latest_.temperatureC = dhtResult.temperatureC;
        latest_.humidityPercent = dhtResult.humidityPercent;
    }

    // 温湿度传感器失败会记录错误，但不阻断其他传感器更新。
    if (!dhtResult.error.isEmpty())
    {
        latest_.hasError = true;
        latest_.errorMessage = dhtResult.error;
        latest_.sensorStale = dhtResult.usedCachedValue;
        static unsigned long lastSensorWarnLogMs = 0;
        if (now - lastSensorWarnLogMs >= kDhtWarnIntervalMs)
        {
            LOG_WARN("SENSOR", "DHT 读取失败，status=%s，温湿度%s",
                     dhtResult.status.c_str(),
                     dhtResult.usedCachedValue ? "保持上次有效值" : "暂无有效值");
            lastSensorWarnLogMs = now;
        }
    }
    else
    {
        latest_.sensorStale = dhtResult.usedCachedValue;
    }

    latest_.lightPercent = light_.readPercent();
    latest_.mq2Percent = mq2_.readPercent();
    latest_.rainPercent = rain_.readPercent();
    latest_.smokeLevel = smokeLevelFromPercent(latest_.mq2Percent);

    // 每30秒输出一次完整的传感器状态
    static unsigned long lastDebugLogMs = 0;
    if (now - lastDebugLogMs > 30000)
    {
        LOG_DEBUG("SENSOR", "传感器状态: T=%.1f°C H=%.1f%% 光=%d%% MQ2=%d%% 雨=%d%% 烟=%s",
                  latest_.temperatureC, latest_.humidityPercent,
                  latest_.lightPercent, latest_.mq2Percent, latest_.rainPercent,
                  latest_.smokeLevel.c_str());
        lastDebugLogMs = now;
    }

    return true;
}

const SensorSnapshot &SensorHub::latest() const
{
    return latest_;
}

String SensorHub::smokeLevelFromPercent(uint8_t percent) const
{
    // 提供给页面与自动化使用的粗粒度风险分级。
    if (percent < 25)
    {
        return "green";
    }
    if (percent < 50)
    {
        return "blue";
    }
    if (percent < 75)
    {
        return "yellow";
    }

    return "red";
}
