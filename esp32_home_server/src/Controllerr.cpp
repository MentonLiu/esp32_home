// 文件说明：esp32_home_server/src/Controllerr.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "Controllerr.h"

namespace
{
    // 常见直流电机驱动板在 1k-5kHz 区间兼容性更好。
    constexpr uint32_t kFanPwmFrequencyHz = 2000;
    constexpr uint8_t kFanPwmResolutionBits = 8;
} // namespace

RelayFanController::RelayFanController(uint8_t pin, uint8_t pwmChannel)
    : pin_(pin), pwmChannel_(pwmChannel)
{
}

void RelayFanController::begin()
{
    // 直流电机调速使用 LEDC PWM。
    // 注意：该引脚应连接 MOSFET/驱动模块，不应直连机械继电器做调速。
    pinMode(pin_, OUTPUT);
    ledcSetup(pwmChannel_, kFanPwmFrequencyHz, kFanPwmResolutionBits);
    ledcAttachPin(pin_, pwmChannel_);
    ledcWrite(pwmChannel_, 0);
    setMode(FanMode::Off);
}

void RelayFanController::setMode(FanMode mode)
{
    // 统一把档位映射到固定速度百分比。
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
    // 百分比换算为 8bit 占空比。
    speedPercent_ = constrain(speedPercent, 0, 100);
    const uint8_t duty = static_cast<uint8_t>(map(speedPercent_, 0, 100, 0, 255));
    ledcWrite(pwmChannel_, duty);
    // 同步更新档位语义，供页面显示。
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
    // 反向映射区间可按硬件体验继续微调。
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
    // 标准舵机 50Hz。
    servoA_.setPeriodHertz(50);
    servoB_.setPeriodHertz(50);
    // 脉宽范围可按舵机型号调整（单位 us）。
    servoA_.attach(pinA_, 500, 2400);
    servoB_.attach(pinB_, 500, 2400);
    setAngle(0);
}

void DualCurtainController::setAngle(uint8_t angle)
{
    // 双舵机反向联动：A 正向，B 反向。
    currentAngle_ = constrain(angle, 0, 180);
    servoA_.write(currentAngle_);
    servoB_.write(180 - currentAngle_);
}

void DualCurtainController::setPresetLevel(uint8_t preset)
{
    // 预设：0=全关,1=1/4,2=半开,3=3/4,4=全开。
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
    // 蜂鸣器使用独立 PWM 通道，默认静音。
    ledcSetup(pwmChannel_, 2000, 8);
    ledcAttachPin(pin_, pwmChannel_);
    ledcWrite(pwmChannel_, 0);
    ledcWriteTone(pwmChannel_, 0);
}

void BuzzerController::beep(uint16_t frequency, uint16_t durationMs)
{
    // 阻塞式蜂鸣：简单直观，适合短告警；复杂场景可改为非阻塞状态机。
    // 显式设置占空比可提升无源蜂鸣器响度。
    ledcWrite(pwmChannel_, 220);
    ledcWriteTone(pwmChannel_, frequency);
    delay(durationMs);
    ledcWriteTone(pwmChannel_, 0);
    ledcWrite(pwmChannel_, 0);
}

void BuzzerController::patternShortShortLong()
{
    // 火警提示节奏：短-短-长。
    beep(2400, 220);
    delay(120);
    beep(2400, 220);
    delay(120);
    beep(1700, 700);
}

IRController::IRController(uint8_t rxPin, uint8_t txPin, uint32_t baudRate)
    : rxPin_(rxPin), txPin_(txPin), baudRate_(baudRate)
{
}

void IRController::begin(Stream &serial)
{
    // 仅持有 Stream 引用，不接管其生命周期。
    serial_ = &serial;
}

bool IRController::sendTextCommand(const String &commandText)
{
    // 统一走按行协议，便于 ESP8266 端解析。
    return sendLine(commandText);
}

IRDecodedSignal IRController::receive()
{
    IRDecodedSignal result;
    if (serial_ == nullptr || !serial_->available())
    {
        return result;
    }

    result.payload = serial_->readStringUntil('\n');
    // 去掉 CR/LF 和两端空白。
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

    // 文本协议一行一条命令。
    serial_->println(line);
    lastCommand_ = line;
    return true;
}
