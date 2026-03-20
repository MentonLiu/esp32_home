#ifndef CONTROLLERR_H
#define CONTROLLERR_H

#include <Arduino.h>
#include <ESP32Servo.h>

#include "SystemContracts.h"

class RelayFanController
{
public:
    explicit RelayFanController(uint8_t pin, uint8_t pwmChannel = 0);

    void begin();
    void setMode(FanMode mode);
    void setSpeedPercent(uint8_t speedPercent);

    FanMode mode() const;
    uint8_t speedPercent() const;

private:
    FanMode modeFromSpeed(uint8_t speedPercent) const;

    uint8_t pin_;
    uint8_t pwmChannel_;
    uint8_t speedPercent_ = 0;
    FanMode mode_ = FanMode::Off;
};

class DualCurtainController
{
public:
    DualCurtainController(uint8_t pinA, uint8_t pinB);

    void begin();
    void setAngle(uint8_t angle);
    void setPresetLevel(uint8_t preset);

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
    explicit BuzzerController(uint8_t pin, uint8_t pwmChannel = 6);

    void begin();
    void beep(uint16_t frequency, uint16_t durationMs);
    void patternShortShortLong();

private:
    uint8_t pin_;
    uint8_t pwmChannel_;
};

struct IRDecodedSignal
{
    bool available = false;
    String payload;
};

class IRController
{
public:
    IRController(uint8_t rxPin, uint8_t txPin, uint32_t baudRate);

    void begin(Stream &serial);
    bool sendProtocol(const String &protocol, uint32_t address, uint32_t command, uint8_t repeats = 0);
    bool sendAction(const String &action, const String &argsJson = "{}");
    bool sendJson(const String &jsonText);
    IRDecodedSignal receive();

    uint32_t baudRate() const;
    String lastCommand() const;

private:
    bool sendLine(const String &line);

    Stream *serial_ = nullptr;
    uint8_t rxPin_;
    uint8_t txPin_;
    uint32_t baudRate_;
    String lastCommand_;
};

#endif
