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
    const char *smokeLevel = "green";
    bool flameDetected = false;
    bool hasError = false;
    String errorMessage;
    unsigned long timestamp = 0;
};

class DHTSensor
{
public:
    DHTSensor(uint8_t pin, uint8_t type);
    void begin();
    bool read(float &temperatureC, float &humidityPercent, String &error);

private:
    DHT dht_;
};

class AnalogPercentSensor
{
public:
    AnalogPercentSensor(uint8_t pin, bool inverted = false);
    void begin() const;
    uint8_t readPercent() const;

private:
    uint8_t pin_;
    bool inverted_;
};

class SensorHub
{
public:
    SensorHub(uint8_t dhtPin, uint8_t dhtType, uint8_t ldrPin, uint8_t mq2Pin, uint8_t flamePin);

    void begin();
    bool update();
    const SensorSnapshot &snapshot() const;

private:
    const char *toSmokeLevel(uint8_t mq2Percent) const;

    DHTSensor dht_;
    AnalogPercentSensor ldr_;
    AnalogPercentSensor mq2_;
    AnalogPercentSensor flame_;
    SensorSnapshot latest_;
    unsigned long lastSampleMs_ = 0;
};

#endif