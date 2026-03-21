// 文件说明：esp32_home_server/src/Sensor.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "Sensor.h"

#include <math.h>

DhtSensor::DhtSensor(uint8_t pin, uint8_t sensorType) : dht_(pin, sensorType) {}

void DhtSensor::begin()
{
    dht_.begin();
}

bool DhtSensor::read(float &temperatureC, float &humidityPercent, String &error)
{
    // 温湿度传感器可能出现瞬时读取失败，是否保留旧值由上层决定。
    temperatureC = dht_.readTemperature();
    humidityPercent = dht_.readHumidity();
    if (isnan(temperatureC) || isnan(humidityPercent))
    {
        error = "dht_read_failed";
        return false;
    }

    return true;
}

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
                     uint8_t flamePin,
                     uint8_t dhtType)
    : dht_(dhtPin, dhtType),
      light_(lightPin, true),
      mq2_(mq2Pin, false),
      flame_(flamePin, false)
{
}

void SensorHub::begin()
{
    // 一次性初始化全部硬件通道。
    dht_.begin();
    light_.begin();
    mq2_.begin();
    flame_.begin();
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

    // 温湿度传感器失败会记录错误，但不阻断其他传感器更新。
    String error;
    if (!dht_.read(latest_.temperatureC, latest_.humidityPercent, error))
    {
        latest_.hasError = true;
        latest_.errorMessage = error;
    }

    latest_.lightPercent = light_.readPercent();
    latest_.mq2Percent = mq2_.readPercent();
    latest_.smokeLevel = smokeLevelFromPercent(latest_.mq2Percent);
    // 火焰检测采用百分比阈值二值判定。
    latest_.flameDetected = flame_.readPercent() >= 60;
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
