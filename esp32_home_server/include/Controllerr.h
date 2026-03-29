// 文件说明：esp32_home_server/include/Controllerr.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef CONTROLLERR_H
#define CONTROLLERR_H

#include <Arduino.h>
#include <ESP32Servo.h>

#include "SystemContracts.h"

// 风扇控制器（历史命名保留为 RelayFanController）：
// 实际实现是“PWM 风扇驱动”，需要外部 MOSFET/电机驱动板，
// 不能直接通过机械继电器实现速度控制。
class RelayFanController
{
public:
    // pin: 风扇 PWM 输出引脚。
    // pwmChannel: LEDC 通道号，需避免和其它模块冲突。
    explicit RelayFanController(uint8_t pin, uint8_t pwmChannel = 0);

    // 初始化 GPIO 与 PWM。
    void begin();
    // 通过档位设置风扇速度。
    void setMode(FanMode mode);
    // 直接设置速度百分比（0-100）。
    void setSpeedPercent(uint8_t speedPercent);

    // 当前档位。
    FanMode mode() const;
    // 当前速度百分比。
    uint8_t speedPercent() const;
    // 当前 PWM 输出是否处于激活状态。
    bool outputActive() const;
    // 当前占空比（0-255）。
    uint8_t pwmDuty() const;
    // 诊断用引脚/通道/频率信息。
    uint8_t pin() const;
    uint8_t pwmChannel() const;
    uint32_t pwmFrequencyHz() const;

private:
    // 根据百分比回推档位，便于状态展示。
    FanMode modeFromSpeed(uint8_t speedPercent) const;

    uint8_t pin_;
    uint8_t pwmChannel_;
    uint8_t speedPercent_ = 0;
    uint8_t pwmDuty_ = 0;
    FanMode mode_ = FanMode::Off;
};

// 双舵机窗帘控制器：
// 两个舵机反向联动，实现单逻辑角度控制。
class DualCurtainController
{
public:
    // pinA/pinB: 两个舵机信号脚。
    DualCurtainController(uint8_t pinA, uint8_t pinB);

    // 设置舵机频率并 attach 到引脚。
    void begin();
    // 维护舵机自动释放状态，避免长期保持带来抖动与额外电流。
    void loop();
    // 直接设置窗帘角度（0-180）。
    void setAngle(uint8_t angle);
    // 设置预设档位（0-4）。
    void setPresetLevel(uint8_t preset);

    // 查询当前角度。
    uint8_t angle() const;

private:
    Servo servoA_;
    Servo servoB_;
    uint8_t pinA_;
    uint8_t pinB_;
    uint8_t currentAngle_ = 0;
    bool attached_ = false;
    unsigned long lastMotionMs_ = 0;
};

// 蜂鸣器控制器，支持单次鸣叫和固定告警模式。
class BuzzerController
{
public:
    // pwmChannel 默认使用独立通道，避免与舵机/风扇冲突。
    explicit BuzzerController(uint8_t pin, uint8_t pwmChannel = 6);

    // 初始化蜂鸣器 PWM。
    void begin();
    // 推进非阻塞蜂鸣状态机。
    void loop();
    // 发出指定频率和时长的蜂鸣。
    void beep(uint16_t frequency, uint16_t durationMs);
    // 固定短短长告警节奏。
    void patternShortShortLong();

private:
    struct Segment
    {
        uint16_t frequency = 0;
        uint16_t durationMs = 0;
        uint8_t duty = 0;
    };

    void startSegment(const Segment &segment);
    bool enqueueSegment(uint16_t frequency, uint16_t durationMs, uint8_t duty = 220);
    bool enqueueSilence(uint16_t durationMs);

    uint8_t pin_;
    uint8_t pwmChannel_;
    static constexpr uint8_t kQueueCapacity = 8;
    Segment queue_[kQueueCapacity];
    uint8_t queueHead_ = 0;
    uint8_t queueTail_ = 0;
    bool segmentActive_ = false;
    unsigned long segmentStartedMs_ = 0;
    Segment activeSegment_;
};

#endif
