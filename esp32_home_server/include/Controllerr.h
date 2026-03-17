/**
 * @file Controllerr.h
 * @brief 执行器控制器头文件
 * 
 * 定义所有执行器设备的控制类：
 * - 风扇继电器控制器 (RelayFanController)
 * - 双窗帘舵机控制器 (DualCurtainController)
 * - 蜂鸣器控制器 (BuzzerController)
 * - 红外桥接控制器 (IRController)
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
 * @brief 红外模块上行消息结构体
 * @details ESP32可读取ESP8266返回的状态或调试信息
 */
struct IRDecodedSignal
{
    bool available = false; ///< 消息是否有效
    String message;         ///< ESP8266返回的完整消息行
};

/**
 * @brief 红外桥接控制器类
 * @details
 * ESP32不直接驱动红外收发器，而是通过UART向ESP8266下发命令。
 * 该类提供协议专用接口和通用扩展接口，便于后续新增协议或自定义动作。
 */
class IRController
{
public:
    /**
     * @brief 构造函数
    * @param rxPin ESP32 UART接收引脚(接ESP8266 TX)
    * @param txPin ESP32 UART发送引脚(接ESP8266 RX)
    * @param baudRate 串口波特率
     */
    IRController(uint8_t rxPin, uint8_t txPin, uint32_t baudRate);

    /**
    * @brief 初始化红外桥接串口
    * @param serial 外部串口对象(建议HardwareSerial(2))
     */
    void begin(Stream &serial);

    /**
    * @brief 发送NEC协议红外信号(桥接到ESP8266)
     * @param address 设备地址
     * @param command 命令码
     * @param repeats 重复发送次数
     */
    bool sendNEC(uint16_t address, uint8_t command, uint8_t repeats = 0);

    /**
    * @brief 发送通用协议命令
    * @param protocol 协议名，例如NEC/SONY/SAMSUNG
    * @param address 地址
    * @param command 命令
    * @param repeats 重复次数
    */
    bool sendProtocolCommand(const String &protocol, uint32_t address, uint32_t command, uint8_t repeats = 0);

    /**
    * @brief 发送扩展动作命令
    * @param action 动作名，例如learn、stop、raw_send等
    * @param args JSON字符串或自定义参数字符串
    */
    bool sendActionCommand(const String &action, const String &args = "{}");

    /**
    * @brief 直接发送完整JSON命令(最高扩展性)
    * @param jsonCommand 完整JSON字符串
    */
    bool sendJsonCommand(const String &jsonCommand);

    /**
     * @brief 读取ESP8266返回的一行消息
     * @return IRDecodedSignal 上行消息
     */
    IRDecodedSignal receive();

    /**
     * @brief 获取最近一次下发命令内容
     */
    String lastCommand() const;

    /**
     * @brief 获取配置的桥接串口波特率
     */
    uint32_t baudRate() const;

private:
    bool sendLine(const String &line);

    Stream *serial_ = nullptr; ///< 指向ESP8266桥接串口
    uint8_t rxPin_;            ///< ESP32 UART RX引脚
    uint8_t txPin_;            ///< ESP32 UART TX引脚
    uint32_t baudRate_;        ///< 串口波特率
    String lastCommand_;       ///< 最近一次下发命令(用于调试)
};

#endif
