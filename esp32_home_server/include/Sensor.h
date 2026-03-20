#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <DHT.h>

struct SensorSnapshot
{
    float temperatureC = 0.0F;
    float humidityPercent = 0.0F;
    uint8_t lightPercent = 0;
    uint8_t mq2Percent = 0;
    String smokeLevel = "green";
    bool flameDetected = false;
    bool hasError = false;
    String errorMessage;
    unsigned long timestamp = 0;
};

class DhtSensor
{
public:
    DhtSensor(uint8_t pin, uint8_t sensorType);

    void begin();
    bool read(float &temperatureC, float &humidityPercent, String &error);

private:
    DHT dht_;
};

class AnalogPercentSensor
{
public:
    AnalogPercentSensor(uint8_t pin, bool inverted = false, uint16_t adcMax = 4095);

    void begin() const;
    uint8_t readPercent() const;

private:
    uint8_t pin_;
    bool inverted_;
    uint16_t adcMax_;
};

class SensorHub
{
public:
    SensorHub(uint8_t dhtPin,
              uint8_t lightPin,
              uint8_t mq2Pin,
              uint8_t flamePin,
              uint8_t dhtType = DHT11);

    void begin();
    bool poll(unsigned long intervalMs = 500);
    const SensorSnapshot &latest() const;

private:
    String smokeLevelFromPercent(uint8_t percent) const;

    DhtSensor dht_;
    AnalogPercentSensor light_;
    AnalogPercentSensor mq2_;
    AnalogPercentSensor flame_;
    SensorSnapshot latest_;
    unsigned long lastSampleMs_ = 0;
};

#endif
