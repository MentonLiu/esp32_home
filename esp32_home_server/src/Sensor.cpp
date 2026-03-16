#include "Sensor.h"

#include <math.h>

DHTSensor::DHTSensor(uint8_t pin, uint8_t type) : dht_(pin, type) {}

void DHTSensor::begin() { dht_.begin(); }

bool DHTSensor::read(float &temperatureC, float &humidityPercent, String &error)
{
    temperatureC = dht_.readTemperature();
    humidityPercent = dht_.readHumidity();

    if (isnan(temperatureC) || isnan(humidityPercent))
    {
        error = "dht_read_failed";
        return false;
    }

    return true;
}

AnalogPercentSensor::AnalogPercentSensor(uint8_t pin, bool inverted) : pin_(pin), inverted_(inverted) {}

void AnalogPercentSensor::begin() const { pinMode(pin_, INPUT); }

uint8_t AnalogPercentSensor::readPercent() const
{
    const int raw = analogRead(pin_);
    int percent = map(raw, 0, 4095, 0, 100);
    if (inverted_)
    {
        percent = 100 - percent;
    }
    percent = constrain(percent, 0, 100);
    return static_cast<uint8_t>(percent);
}

SensorHub::SensorHub(uint8_t dhtPin, uint8_t dhtType, uint8_t ldrPin, uint8_t mq2Pin, uint8_t flamePin)
    : dht_(dhtPin, dhtType), ldr_(ldrPin, true), mq2_(mq2Pin, false), flame_(flamePin, false) {}

void SensorHub::begin()
{
    dht_.begin();
    ldr_.begin();
    mq2_.begin();
    flame_.begin();
}

bool SensorHub::update()
{
    if (millis() - lastSampleMs_ < 500)
    {
        return false;
    }

    lastSampleMs_ = millis();
    latest_.timestamp = millis();
    latest_.hasError = false;
    latest_.errorMessage = "";

    String dhtError;
    if (!dht_.read(latest_.temperatureC, latest_.humidityPercent, dhtError))
    {
        latest_.hasError = true;
        latest_.errorMessage = dhtError;
    }

    latest_.lightPercent = ldr_.readPercent();
    latest_.mq2Percent = mq2_.readPercent();
    latest_.smokeLevel = toSmokeLevel(latest_.mq2Percent);

    const uint8_t flamePercent = flame_.readPercent();
    latest_.flameDetected = flamePercent >= 60;

    return true;
}

const SensorSnapshot &SensorHub::snapshot() const { return latest_; }

const char *SensorHub::toSmokeLevel(uint8_t mq2Percent) const
{
    if (mq2Percent < 25)
    {
        return "green";
    }
    if (mq2Percent < 50)
    {
        return "blue";
    }
    if (mq2Percent < 75)
    {
        return "yellow";
    }
    return "red";
}
