// 文件说明：esp32_home_server/include/pins.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

namespace pins
{
    // 传感器输入引脚。
    // DHT11 数据线。
    static constexpr uint8_t DHT_DATA = 13;
    // 光照传感器模拟输入。
    static constexpr uint8_t LIGHT_ANALOG = 4;
    // MQ2 烟雾传感器模拟输入。
    static constexpr uint8_t MQ2_ANALOG = 5;

    // 执行器输出引脚。
    // 风扇 PWM 输出（建议连接 ULN2003A 的 IN1）。
    // 注意：不要和 IR 串口 RX 的 GPIO15 复用。
    static constexpr uint8_t FAN_PWM = 18;
    // 窗帘舵机 A 控制脚。
    static constexpr uint8_t CURTAIN_SERVO_A = 16;
    // 窗帘舵机 B 控制脚。
    static constexpr uint8_t CURTAIN_SERVO_B = 17;
    // 蜂鸣器输出脚。
    static constexpr uint8_t BUZZER = 12;

} // 命名空间结束

#endif
