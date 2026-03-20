#include "Controllerr.h"

RelayFanController::RelayFanController(uint8_t pin, uint8_t pwmChannel)
    : pin_(pin), pwmChannel_(pwmChannel)
{
}

void RelayFanController::begin()
{
    // 高频脉宽调制可将风扇控制频率避开可闻范围。
    pinMode(pin_, OUTPUT);
    ledcSetup(pwmChannel_, 25000, 8);
    ledcAttachPin(pin_, pwmChannel_);
    setMode(FanMode::Off);
}

void RelayFanController::setMode(FanMode mode)
{
    switch (mode)
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
    // 将百分比限幅并映射到 8 位占空比。
    speedPercent_ = constrain(speedPercent, 0, 100);
    const uint8_t duty = static_cast<uint8_t>(map(speedPercent_, 0, 100, 0, 255));
    ledcWrite(pwmChannel_, duty);
    mode_ = modeFromSpeed(speedPercent_);
}

FanMode RelayFanController::mode() const
{
    return mode_;
}

uint8_t RelayFanController::speedPercent() const
{
    return speedPercent_;
}

FanMode RelayFanController::modeFromSpeed(uint8_t speedPercent) const
{
    if (speedPercent == 0)
    {
        return FanMode::Off;
    }
    if (speedPercent <= 45)
    {
        return FanMode::Low;
    }
    if (speedPercent <= 80)
    {
        return FanMode::Medium;
    }

    return FanMode::High;
}

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
    // 第二路舵机镜像动作，保证双侧窗帘同步。
    currentAngle_ = constrain(angle, 0, 180);
    servoA_.write(currentAngle_);
    servoB_.write(180 - currentAngle_);
}

void DualCurtainController::setPresetLevel(uint8_t preset)
{
    static const uint8_t kPresetAngles[] = {0, 45, 90, 135, 180};
    setAngle(kPresetAngles[constrain(preset, 0, 4)]);
}

uint8_t DualCurtainController::angle() const
{
    return currentAngle_;
}

BuzzerController::BuzzerController(uint8_t pin, uint8_t pwmChannel) : pin_(pin), pwmChannel_(pwmChannel) {}

void BuzzerController::begin()
{
    ledcSetup(pwmChannel_, 2000, 8);
    ledcAttachPin(pin_, pwmChannel_);
    ledcWriteTone(pwmChannel_, 0);
}

void BuzzerController::beep(uint16_t frequency, uint16_t durationMs)
{
    // 简单阻塞式鸣叫，用于安全告警场景。
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

IRController::IRController(uint8_t rxPin, uint8_t txPin, uint32_t baudRate)
    : rxPin_(rxPin), txPin_(txPin), baudRate_(baudRate)
{
}

void IRController::begin(Stream &serial)
{
    serial_ = &serial;
}

bool IRController::sendProtocol(const String &protocol, uint32_t address, uint32_t command, uint8_t repeats)
{
    // 构造桥接模块可识别的结构化命令行。
    const String payload = String("{\"device\":\"ir\",\"action\":\"send\",\"protocol\":\"") + protocol +
                           "\",\"address\":" + String(address) +
                           ",\"command\":" + String(command) +
                           ",\"repeats\":" + String(repeats) + "}";
    return sendJson(payload);
}

bool IRController::sendAction(const String &action, const String &argsJson)
{
    const String payload = String("{\"device\":\"ir\",\"action\":\"") + action + "\",\"args\":" + argsJson + "}";
    return sendJson(payload);
}

bool IRController::sendJson(const String &jsonText)
{
    return sendLine(jsonText);
}

IRDecodedSignal IRController::receive()
{
    IRDecodedSignal result;
    if (serial_ == nullptr || !serial_->available())
    {
        return result;
    }

    // 桥接模块按行返回结构化消息帧。
    result.payload = serial_->readStringUntil('\n');
    result.payload.trim();
    result.available = result.payload.length() > 0;
    return result;
}

uint32_t IRController::baudRate() const
{
    return baudRate_;
}

String IRController::lastCommand() const
{
    return lastCommand_;
}

bool IRController::sendLine(const String &line)
{
    if (serial_ == nullptr)
    {
        return false;
    }

    // 缓存最近命令，便于诊断与状态展示。
    serial_->println(line);
    lastCommand_ = line;
    return true;
}
