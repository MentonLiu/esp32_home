/**
 * @file Sensor.cpp
 * @brief 传感器实现文件
 * 
 * 实现所有传感器类的具体功能
 */

#include "Sensor.h"

#include <math.h>

// ============ DHTSensor 实现 ============

DHTSensor::DHTSensor(uint8_t pin) : dht_(pin, DHT11) {}

void DHTSensor::begin() { dht_.begin(); }

/**
 * @brief 读取温湿度数据
 * @return bool 读取是否成功(失败时error会包含错误信息)
 */
bool DHTSensor::read(float &temperatureC, float &humidityPercent, String &error)
{
    temperatureC = dht_.readTemperature();   // 读取温度
    humidityPercent = dht_.readHumidity();   // 读取湿度

    // 检查是否为有效数值(isnan表示读取失败)
    if (isnan(temperatureC) || isnan(humidityPercent))
    {
        error = "dht_read_failed";
        return false;
    }

    return true;
}


// ============ AnalogPercentSensor 实现 ============

AnalogPercentSensor::AnalogPercentSensor(uint8_t pin, bool inverted) : pin_(pin), inverted_(inverted) {}

void AnalogPercentSensor::begin() const { pinMode(pin_, INPUT); }

/**
 * @brief 读取模拟传感器并转换为百分比
 * @details ESP32 ADC分辨率12位(0-4095)，映射到0-100%
 * @return uint8_t 百分比值
 */
uint8_t AnalogPercentSensor::readPercent() const
{
    const int raw = analogRead(pin_);  // 读取原始ADC值(0-4095)
    // 映射到百分比(0-100)
    int percent = map(raw, 0, 4095, 0, 100);
    
    // 翻转处理(光敏电阻等低电平表示高值)
    if (inverted_)
    {
        percent = 100 - percent;
    }
    
    percent = constrain(percent, 0, 100);  // 确保在有效范围内
    return static_cast<uint8_t>(percent);
}


// ============ SensorHub 实现 ============

/**
 * @brief 构造函数
 * @details 初始化所有子传感器，LDR配置为翻转模式(光线越强值越高)
 */
SensorHub::SensorHub(uint8_t dhtPin, uint8_t ldrPin, uint8_t mq2Pin, uint8_t flamePin)
    : dht_(dhtPin), ldr_(ldrPin, true), mq2_(mq2Pin, false), flame_(flamePin, false) {}

void SensorHub::begin()
{
    dht_.begin();
    ldr_.begin();
    mq2_.begin();
    flame_.begin();
}

/**
 * @brief 更新所有传感器数据
 * @details 采样间隔500ms，防止频繁读取导致数据不稳定
 * @return bool 是否成功执行(受采样间隔控制)
 */
bool SensorHub::update()
{
    // 采样间隔控制，每500ms更新一次
    if (millis() - lastSampleMs_ < 500)
    {
        return false;
    }

    lastSampleMs_ = millis();  // 更新时间戳
    latest_.timestamp = millis();
    latest_.hasError = false;
    latest_.errorMessage = "";

    // 读取DHT温湿度
    String dhtError;
    if (!dht_.read(latest_.temperatureC, latest_.humidityPercent, dhtError))
    {
        latest_.hasError = true;
        latest_.errorMessage = dhtError;
    }

    // 读取模拟传感器
    latest_.lightPercent = ldr_.readPercent();   // 光照强度
    latest_.mq2Percent = mq2_.readPercent();    // 烟雾浓度
    latest_.smokeLevel = toSmokeLevel(latest_.mq2Percent);  // 转换为烟雾等级

    // 火焰检测：超过60%阈值认为检测到火焰
    const uint8_t flamePercent = flame_.readPercent();
    latest_.flameDetected = flamePercent >= 60;

    return true;
}

const SensorSnapshot &SensorHub::snapshot() const { return latest_; }

/**
 * @brief 将MQ2传感器百分比转换为烟雾等级
 * @return const char* 等级字符串 (green/blue/yellow/red)
 */
const char *SensorHub::toSmokeLevel(uint8_t mq2Percent) const
{
    if (mq2Percent < 25)
    {
        return "green";   // 安全
    }
    if (mq2Percent < 50)
    {
        return "blue";    // 正常
    }
    if (mq2Percent < 75)
    {
        return "yellow";  // 警告
    }
    return "red";        // 危险
}
