#ifndef CONTROLLERR_H
#define CONTROLLERR_H

#include <Arduino.h>
#include <ESP32Servo.h>

#include "SystemContracts.h"

// 基于脉宽调制的继电器风扇驱动，支持档位与速度百分比两种接口。
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
    // 将任意速度百分比映射到标准风扇档位区间。
    FanMode modeFromSpeed(uint8_t speedPercent) const;

    uint8_t pin_;
    uint8_t pwmChannel_;
    uint8_t speedPercent_ = 0;
    FanMode mode_ = FanMode::Off;
};

// 双舵机窗帘驱动。第二路舵机镜像第一路舵机以实现对拉。
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

// 当前实现使用短时阻塞鸣叫（简单可靠）。
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

// 从红外桥接串口读取到的一条解码消息。
struct IRDecodedSignal
{
    bool available = false;
    String payload;
};

// 红外收发文本协议的串口桥接封装。
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
    // 向桥接模块发送一行命令，并缓存最近一次命令。
    bool sendLine(const String &line);

    Stream *serial_ = nullptr;
    uint8_t rxPin_;
    uint8_t txPin_;
    uint32_t baudRate_;
    String lastCommand_;
};

#endif
