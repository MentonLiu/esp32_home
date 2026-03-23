// 文件说明：esp32_home_server/include/Controllerr.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef CONTROLLERR_H
#define CONTROLLERR_H

#include <Arduino.h>
#include <ESP32Servo.h>

#include "SystemContracts.h"

// 风扇控制器：
// 通过 LEDC PWM 控制风扇占空比，向上层暴露“模式 + 百分比”双接口。
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

private:
    // 根据百分比回推档位，便于状态展示。
    FanMode modeFromSpeed(uint8_t speedPercent) const;

    uint8_t pin_;
    uint8_t pwmChannel_;
    uint8_t speedPercent_ = 0;
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
};

// 蜂鸣器控制器，支持单次鸣叫和固定告警模式。
class BuzzerController
{
public:
    // pwmChannel 默认使用独立通道，避免与舵机/风扇冲突。
    explicit BuzzerController(uint8_t pin, uint8_t pwmChannel = 6);

    // 初始化蜂鸣器 PWM。
    void begin();
    // 发出指定频率和时长的蜂鸣。
    void beep(uint16_t frequency, uint16_t durationMs);
    // 固定短短长告警节奏。
    void patternShortShortLong();

private:
    uint8_t pin_;
    uint8_t pwmChannel_;
};

// 红外桥接上行数据。
struct IRDecodedSignal
{
    bool available = false;
    String payload;
};

// 红外桥接控制器：
// 本模块不关心红外编码细节，只转发字符串到外部 ESP8266。
class IRController
{
public:
    IRController(uint8_t rxPin, uint8_t txPin, uint32_t baudRate);

    // 注入串口流对象（通常为 HardwareSerial）。
    void begin(Stream &serial);
    // 发送文本命令到桥接端。
    bool sendTextCommand(const String &commandText);
    // 非阻塞读取桥接回传文本。
    IRDecodedSignal receive();

    // 查询串口波特率。
    uint32_t baudRate() const;
    // 最近一次成功下发的命令。
    String lastCommand() const;

private:
    // 统一行发送实现，自动追加换行。
    bool sendLine(const String &line);

    Stream *serial_ = nullptr;
    uint8_t rxPin_;
    uint8_t txPin_;
    uint32_t baudRate_;
    String lastCommand_;
};

#endif
