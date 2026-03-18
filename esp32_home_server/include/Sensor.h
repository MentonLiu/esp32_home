/**
 * @file Sensor.h
 * @brief 传感器头文件
 *
 * 定义所有传感器相关的类：
 * - DHTSensor: DHT11温湿度传感器
 * - AnalogPercentSensor: 模拟量传感器(光敏、烟雾、火焰)
 * - SensorHub: 传感器集线器，统一管理所有传感器
 *
 * 传感器数据通过SensorSnapshot结构体封装
 */

#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <DHT.h>

/**
 * @brief 传感器数据快照结构体
 * 存储所有传感器读取的数据
 */
struct SensorSnapshot
{
    float temperatureC = 0.0F;        ///< 温度 (摄氏度)
    float humidityPercent = 0.0F;     ///< 湿度 (百分比)
    uint8_t lightPercent = 0;         ///< 光照强度 (百分比)
    uint8_t mq2Percent = 0;           ///< 烟雾浓度 (百分比)
    const char *smokeLevel = "green"; ///< 烟雾等级 (green/blue/yellow/red)
    bool flameDetected = false;       ///< 是否检测到火焰
    bool hasError = false;            ///< 是否有错误
    String errorMessage;              ///< 错误信息
    unsigned long timestamp = 0;      ///< 采样时间戳
};

/**
 * @brief DHT温湿度传感器类
 * @details 固定使用DHT11型号
 */
class DHTSensor
{
public:
    /**
     * @brief 构造函数
     * @param pin 数据引脚
     */
    explicit DHTSensor(uint8_t pin);

    /**
     * @brief 初始化传感器
     */
    void begin();

    /**
     * @brief 读取温湿度数据
     * @param temperatureC 输出：温度(摄氏度)
     * @param humidityPercent 输出：湿度(百分比)
     * @param error 输出：错误信息
     * @return bool 是否读取成功
     */
    bool read(float &temperatureC, float &humidityPercent, String &error);

private:
    DHT dht_; ///< DHT库传感器对象
};

/**
 * @brief 模拟量百分比传感器类
 * @details 将ADC值转换为0-100%的百分比，支持翻转
 */
class AnalogPercentSensor
{
public:
    /**
     * @brief 构造函数
     * @param pin 模拟输入引脚
     * @param inverted 是否翻转读数(部分传感器高电平对应低值)
     */
    AnalogPercentSensor(uint8_t pin, bool inverted = false);

    /**
     * @brief 初始化传感器
     */
    void begin() const;

    /**
     * @brief 读取并转换为百分比
     * @return uint8_t 百分比值 (0-100)
     */
    uint8_t readPercent() const;

private:
    uint8_t pin_;   ///< 模拟输入引脚
    bool inverted_; ///< 是否翻转
};

/**
 * @brief 传感器集线器类
 * @details 统一管理所有传感器，提供统一的采样接口
 */
class SensorHub
{
public:
    /**
     * @brief 构造函数
     * @param dhtPin DHT传感器引脚
     * @param ldrPin 光敏电阻引脚
     * @param mq2Pin MQ2烟雾传感器引脚
     * @param flamePin 火焰传感器引脚
     */
    SensorHub(uint8_t dhtPin, uint8_t ldrPin, uint8_t mq2Pin, uint8_t flamePin);

    /**
     * @brief 初始化所有传感器
     */
    void begin();

    /**
     * @brief 更新传感器数据
     * @return bool 是否成功读取(受采样间隔控制)
     */
    bool update();

    /**
     * @brief 获取最新传感器快照
     * @return const SensorSnapshot& 传感器数据引用
     */
    const SensorSnapshot &snapshot() const;

private:
    /**
     * @brief 将MQ2百分比转换为烟雾等级
     * @param mq2Percent MQ2读数百分比
     * @return const char* 等级字符串
     */
    const char *toSmokeLevel(uint8_t mq2Percent) const;

    DHTSensor dht_;                  ///< 温湿度传感器
    AnalogPercentSensor ldr_;        ///< 光敏传感器(光强)
    AnalogPercentSensor mq2_;        ///< MQ2烟雾传感器
    AnalogPercentSensor flame_;      ///< 火焰传感器
    SensorSnapshot latest_;          ///< 最新传感器数据
    unsigned long lastSampleMs_ = 0; ///< 上次采样时间
};

#endif
