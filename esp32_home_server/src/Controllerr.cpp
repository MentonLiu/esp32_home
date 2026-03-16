#include "Controllerr.h"

#include <IRremote.hpp>

RelayFanController::RelayFanController(uint8_t pin) : pin_(pin) {}

void RelayFanController::begin()
{
    pinMode(pin_, OUTPUT);
    ledcSetup(pwmChannel_, 25000, 8);
    ledcAttachPin(pin_, pwmChannel_);
    setMode(FanMode::Off);
}

void RelayFanController::setMode(FanMode mode)
{
    mode_ = mode;
    switch (mode_)
    {
    case FanMode::Off:
        setSpeedPercent(0);
        break;
    case FanMode::Low:
        setSpeedPercent(30);
        break;
    case FanMode::Medium:
        setSpeedPercent(65);
        break;
    case FanMode::High:
        setSpeedPercent(100);
        break;
    }
}

void RelayFanController::setSpeedPercent(uint8_t speedPercent)
{
    speedPercent_ = constrain(speedPercent, 0, 100);
    const uint8_t duty = map(speedPercent_, 0, 100, 0, 255);
    ledcWrite(pwmChannel_, duty);

    if (speedPercent_ == 0)
    {
        mode_ = FanMode::Off;
    }
}

FanMode RelayFanController::mode() const { return mode_; }

uint8_t RelayFanController::speedPercent() const { return speedPercent_; }

DualCurtainController::DualCurtainController(uint8_t pinA, uint8_t pinB) : pinA_(pinA), pinB_(pinB) {}

void DualCurtainController::begin()
{
    servoA_.setPeriodHertz(50);
    servoB_.setPeriodHertz(50);
    servoA_.attach(pinA_, 500, 2400);
    servoB_.attach(pinB_, 500, 2400);
    setAngle(0);
}

void DualCurtainController::setAngle(uint8_t angle)
{
    currentAngle_ = constrain(angle, 0, 180);
    servoA_.write(currentAngle_);
    servoB_.write(180 - currentAngle_);
}

void DualCurtainController::setPresetLevel(uint8_t level)
{
    static const uint8_t presets[] = {0, 45, 90, 135, 180};
    level = constrain(level, 0, 4);
    setAngle(presets[level]);
}

uint8_t DualCurtainController::angle() const { return currentAngle_; }

BuzzerController::BuzzerController(uint8_t pin) : pin_(pin) {}

void BuzzerController::begin()
{
    ledcSetup(pwmChannel_, 2000, 8);
    ledcAttachPin(pin_, pwmChannel_);
    ledcWriteTone(pwmChannel_, 0);
}

void BuzzerController::beep(uint16_t frequency, uint16_t durationMs)
{
    ledcWriteTone(pwmChannel_, frequency);
    delay(durationMs);
    ledcWriteTone(pwmChannel_, 0);
}

void BuzzerController::patternShortShortLong()
{
    beep(2400, 120);
    delay(90);
    beep(2400, 120);
    delay(90);
    beep(1700, 450);
}

IRController::IRController(uint8_t rxPin, uint8_t txPin) : rxPin_(rxPin), txPin_(txPin) {}

void IRController::begin()
{
    IrReceiver.begin(rxPin_, DISABLE_LED_FEEDBACK);
    IrSender.begin(txPin_);
}

void IRController::sendNEC(uint16_t address, uint8_t command, uint8_t repeats) const
{
    IrSender.sendNEC(address, command, repeats);
}

IRDecodedSignal IRController::receive()
{
    IRDecodedSignal result;
    if (!IrReceiver.decode())
    {
        return result;
    }

    result.available = true;
    result.protocol = protocolToString(IrReceiver.decodedIRData.protocol);
    result.address = IrReceiver.decodedIRData.address;
    result.command = IrReceiver.decodedIRData.command;
    result.rawData = IrReceiver.decodedIRData.decodedRawData;

    IrReceiver.resume();
    return result;
}

String IRController::protocolToString(uint8_t protocol) const
{
    switch (protocol)
    {
    case NEC:
        return "NEC";
    case SONY:
        return "SONY";
    case SAMSUNG:
        return "SAMSUNG";
    case RC5:
        return "RC5";
    case RC6:
        return "RC6";
    default:
        return "UNKNOWN";
    }
}
