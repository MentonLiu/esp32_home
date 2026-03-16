/**
 * @file Controllerr.h
 * @brief 执行器控制器头文件
 * 
 * 定义所有执行器设备的控制类：
 * - 风扇继电器控制器 (RelayFanController)
 * - 双窗帘舵机控制器 (DualCurtainController)
 * - 蜂鸣器控制器 (BuzzerController)
 * - 红外遥控控制器 (IRController)
 */

#ifndef CONTROLLERR_H
#define CONTROLLERR_H

#include <Arduino.h>
#include <ESP32Servo.h>

/**
 * @brief 风扇运行模式枚举
 * 通过不同占空比实现风扇调速
 */
enum class FanMode : uint8_t
{
    Off = 0,    ///< 关闭
    Low = 1,    ///< 低速 (30%占空比)
    Medium = 2, ///< 中速 (65%占空比)
    High = 3    ///< 高速 (100%占空比)
};

/**
 * @brief 风扇继电器控制器类
 * @details 使用PWM控制继电器模块，实现风扇无级调速
 */
class RelayFanController
{
public:
    /**
     * @brief 构造函数
     * @param pin 继电器控制引脚
     */
    explicit RelayFanController(uint8_t pin);

    /**
     * @brief 初始化控制器
     * 配置GPIO模式和PWM参数
     */
    void begin();

    /**
     * @brief 设置风扇运行模式
     * @param mode 风扇模式 (Off/Low/Medium/High)
     */
    void setMode(FanMode mode);

    /**
     * @brief 自定义风扇转速
     * @param speedPercent 转速百分比 (0-100)
     */
    void setSpeedPercent(uint8_t speedPercent);

    /**
     * @brief 获取当前风扇模式
     * @return FanMode 当前模式
     */
    FanMode mode() const;

    /**
     * @brief 获取当前转速百分比
     * @return uint8_t 转速百分比 (0-100)
     */
    uint8_t speedPercent() const;

private:
    uint8_t pin_;              ///< 继电器控制引脚
    uint8_t speedPercent_ = 0; ///< 当前转速百分比
    FanMode mode_ = FanMode::Off; ///< 当前运行模式
    uint8_t pwmChannel_ = 0;   ///< PWM通道号
};

/**
 * @brief 双窗帘舵机控制器类
 * @details 使用两个舵机分别控制左右两侧窗帘，实现同步开合
 */
class DualCurtainController
{
public:
    /**
     * @brief 构造函数
     * @param pinA 左侧舵机引脚
     * @param pinB 右侧舵机引脚
     */
    DualCurtainController(uint8_t pinA, uint8_t pinB);

    /**
     * @brief 初始化舵机
     * 配置舵机PWM参数和初始角度
     */
    void begin();

    /**
     * @brief 设置窗帘角度
     * @param angle 角度值 (0-180度)，0为全关，180为全开
     */
    void setAngle(uint8_t angle);

    /**
     * @brief 设置预设开合等级
     * @param level 预设等级 (0-4)，对应不同的开合程度
     */
    void setPresetLevel(uint8_t level);

    /**
     * @brief 获取当前窗帘角度
     * @return uint8_t 当前角度值
     */
    uint8_t angle() const;

private:
    Servo servoA_;             ///< 左侧舵机对象
    Servo servoB_;             ///< 右侧舵机对象
    uint8_t pinA_;             ///< 左侧舵机引脚
    uint8_t pinB_;             ///< 右侧舵机引脚
    uint8_t currentAngle_ = 0; ///< 当前角度
};

/**
 * @brief 蜂鸣器控制器类
 * @details 使用PWM产生不同频率的声音，用于报警和提示
 */
class BuzzerController
{
public:
    /**
     * @brief 构造函数
     * @param pin 蜂鸣器控制引脚
     */
    explicit BuzzerController(uint8_t pin);

    /**
     * @brief 初始化蜂鸣器
     * 配置PWM参数
     */
    void begin();

    /**
     * @brief 发出蜂鸣声
     * @param frequency 频率 (Hz)
     * @param durationMs 持续时间 (毫秒)
     */
    void beep(uint16_t frequency, uint16_t durationMs);

    /**
     * @brief 播放短-短-长警报模式
     * @details 用于火焰警报，典型模式：短促-短促-长声
     */
    void patternShortShortLong();

private:
    uint8_t pin_;             ///< 蜂鸣器引脚
    uint8_t pwmChannel_ = 6;  ///< PWM通道号
};

/**
 * @brief 红外解码信号结构体
 * 存储接收到的红外遥控信号数据
 */
struct IRDecodedSignal
{
    bool available = false;      ///< 信号是否有效
    String protocol;             ///< 红外协议名称 (NEC/SONY/SAMSUNG等)
    uint16_t address = 0;        ///< 设备地址
    uint16_t command = 0;       ///< 命令码
    uint32_t rawData = 0;        ///< 原始数据
};

/**
 * @brief 红外遥控控制器类
 * @details 支持红外信号的接收和发送，可控制空调、电视等红外设备
 */
class IRController
{
public:
    /**
     * @brief 构造函数
     * @param rxPin 红外接收模块引脚
     * @param txPin 红外发射模块引脚
     */
    IRController(uint8_t rxPin, uint8_t txPin);

    /**
     * @brief 初始化红外模块
     * 启动红外接收和发送功能
     */
    void begin();

    /**
     * @brief 发送NEC协议红外信号
     * @param address 设备地址
     * @param command 命令码
     * @param repeats 重复发送次数
     */
    void sendNEC(uint16_t address, uint8_t command, uint8_t repeats = 0) const;

    /**
     * @brief 接收红外信号
     * @return IRDecodedSignal 解码后的红外信号数据
     */
    IRDecodedSignal receive();

private:
    /**
     * @brief 将协议编号转换为字符串
     * @param protocol 协议编号
     * @return String 协议名称
     */
    String protocolToString(uint8_t protocol) const;

    uint8_t rxPin_;  ///< 红外接收引脚
    uint8_t txPin_;  ///< 红外发射引脚
};

#endif
