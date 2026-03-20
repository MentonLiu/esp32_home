#include "Sensor.h"

#include <math.h>

DhtSensor::DhtSensor(uint8_t pin, uint8_t sensorType) : dht_(pin, sensorType) {}

void DhtSensor::begin()
{
    dht_.begin();
}

bool DhtSensor::read(float &temperatureC, float &humidityPercent, String &error)
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

AnalogPercentSensor::AnalogPercentSensor(uint8_t pin, bool inverted, uint16_t adcMax)
    : pin_(pin), inverted_(inverted), adcMax_(adcMax) {}

void AnalogPercentSensor::begin() const
{
    pinMode(pin_, INPUT);
}

uint8_t AnalogPercentSensor::readPercent() const
{
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
    dht_.begin();
    light_.begin();
    mq2_.begin();
    flame_.begin();
}

bool SensorHub::poll(unsigned long intervalMs)
{
    const unsigned long now = millis();
    if (now - lastSampleMs_ < intervalMs)
    {
        return false;
    }

    lastSampleMs_ = now;
    latest_.timestamp = now;
    latest_.hasError = false;
    latest_.errorMessage = "";

    String error;
    if (!dht_.read(latest_.temperatureC, latest_.humidityPercent, error))
    {
        latest_.hasError = true;
        latest_.errorMessage = error;
    }

    latest_.lightPercent = light_.readPercent();
    latest_.mq2Percent = mq2_.readPercent();
    latest_.smokeLevel = smokeLevelFromPercent(latest_.mq2Percent);
    latest_.flameDetected = flame_.readPercent() >= 60;
    return true;
}

const SensorSnapshot &SensorHub::latest() const
{
    return latest_;
}

String SensorHub::smokeLevelFromPercent(uint8_t percent) const
{
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
