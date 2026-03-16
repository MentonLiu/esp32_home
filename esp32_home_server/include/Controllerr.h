#ifndef CONTROLLERR_H
#define CONTROLLERR_H

#include <Arduino.h>
#include <ESP32Servo.h>

enum class FanMode : uint8_t
{
    Off = 0,
    Low = 1,
    Medium = 2,
    High = 3
};

class RelayFanController
{
public:
    explicit RelayFanController(uint8_t pin);

    void begin();
    void setMode(FanMode mode);
    void setSpeedPercent(uint8_t speedPercent);
    FanMode mode() const;
    uint8_t speedPercent() const;

private:
    uint8_t pin_;
    uint8_t speedPercent_ = 0;
    FanMode mode_ = FanMode::Off;
    uint8_t pwmChannel_ = 0;
};

class DualCurtainController
{
public:
    DualCurtainController(uint8_t pinA, uint8_t pinB);

    void begin();
    void setAngle(uint8_t angle);
    void setPresetLevel(uint8_t level);
    uint8_t angle() const;

private:
    Servo servoA_;
    Servo servoB_;
    uint8_t pinA_;
    uint8_t pinB_;
    uint8_t currentAngle_ = 0;
};

class BuzzerController
{
public:
    explicit BuzzerController(uint8_t pin);

    void begin();
    void beep(uint16_t frequency, uint16_t durationMs);
    void patternShortShortLong();

private:
    uint8_t pin_;
    uint8_t pwmChannel_ = 6;
};

struct IRDecodedSignal
{
    bool available = false;
    String protocol;
    uint16_t address = 0;
    uint16_t command = 0;
    uint32_t rawData = 0;
};

class IRController
{
public:
    IRController(uint8_t rxPin, uint8_t txPin);

    void begin();
    void sendNEC(uint16_t address, uint8_t command, uint8_t repeats = 0) const;
    IRDecodedSignal receive();

private:
    String protocolToString(uint8_t protocol) const;

    uint8_t rxPin_;
    uint8_t txPin_;
};

#endif