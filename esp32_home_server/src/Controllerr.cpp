// 文件说明：esp32_home_server/src/Controllerr.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "Controllerr.h"

namespace
{
    // 常见直流电机驱动板在 1k-5kHz 区间兼容性更好。
    constexpr uint32_t kFanPwmFrequencyHz = 2000;
    constexpr uint8_t kFanPwmResolutionBits = 8;
    constexpr unsigned long kCurtainServoHoldMs = 450;
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
    setAngle(0);
}

void DualCurtainController::loop()
{
    // 到位后释放 PWM，减小供电压力和“被其它负载带着抖”的概率。
    if (attached_ && millis() - lastMotionMs_ >= kCurtainServoHoldMs)
    {
        servoA_.detach();
        servoB_.detach();
        attached_ = false;
    }
}

void DualCurtainController::setAngle(uint8_t angle)
{
    if (!attached_)
    {
        // 仅在需要动作时重新占用定时资源。
        servoA_.attach(pinA_, 500, 2400);
        servoB_.attach(pinB_, 500, 2400);
        attached_ = true;
    }

    // 双舵机反向联动：A 正向，B 反向。
    currentAngle_ = constrain(angle, 0, 180);
    servoA_.write(currentAngle_);
    servoB_.write(180 - currentAngle_);
    lastMotionMs_ = millis();
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

void BuzzerController::loop()
{
    if (segmentActive_ && millis() - segmentStartedMs_ >= activeSegment_.durationMs)
    {
        segmentActive_ = false;
        ledcWriteTone(pwmChannel_, 0);
        ledcWrite(pwmChannel_, 0);
    }

    if (segmentActive_ || queueHead_ == queueTail_)
    {
        return;
    }

    startSegment(queue_[queueHead_]);
    queueHead_ = (queueHead_ + 1) % kQueueCapacity;
}

void BuzzerController::beep(uint16_t frequency, uint16_t durationMs)
{
    // 非阻塞入队，避免告警期间卡住 Web、自动化和网络栈。
    enqueueSegment(frequency, durationMs);
}

void BuzzerController::patternShortShortLong()
{
    // 火警提示节奏：短-短-长。
    enqueueSegment(2400, 220);
    enqueueSilence(120);
    enqueueSegment(2400, 220);
    enqueueSilence(120);
    enqueueSegment(1700, 700);
}

void BuzzerController::startSegment(const Segment &segment)
{
    activeSegment_ = segment;
    segmentStartedMs_ = millis();
    segmentActive_ = true;

    if (segment.frequency == 0 || segment.duty == 0)
    {
        ledcWriteTone(pwmChannel_, 0);
        ledcWrite(pwmChannel_, 0);
        return;
    }

    ledcWrite(pwmChannel_, segment.duty);
    ledcWriteTone(pwmChannel_, segment.frequency);
}

bool BuzzerController::enqueueSegment(uint16_t frequency, uint16_t durationMs, uint8_t duty)
{
    const uint8_t nextTail = (queueTail_ + 1) % kQueueCapacity;
    if (nextTail == queueHead_)
    {
        return false;
    }

    queue_[queueTail_].frequency = frequency;
    queue_[queueTail_].durationMs = durationMs;
    queue_[queueTail_].duty = duty;
    queueTail_ = nextTail;
    return true;
}

bool BuzzerController::enqueueSilence(uint16_t durationMs)
{
    return enqueueSegment(0, durationMs, 0);
}
