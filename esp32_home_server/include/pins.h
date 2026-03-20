#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

namespace pins
{
    // 传感器输入引脚。
    static constexpr uint8_t DHT_DATA = 13;
    static constexpr uint8_t LIGHT_ANALOG = 4;
    static constexpr uint8_t MQ2_ANALOG = 5;
    static constexpr uint8_t FLAME_ANALOG = 6;

    // 执行器输出引脚。
    static constexpr uint8_t FAN_PWM = 18;
    static constexpr uint8_t CURTAIN_SERVO_A = 16;
    static constexpr uint8_t CURTAIN_SERVO_B = 17;
    static constexpr uint8_t BUZZER = 12;

    // 连接红外桥接控制器的串口引脚。
    static constexpr uint8_t IR_BRIDGE_UART_RX = 15;
    static constexpr uint8_t IR_BRIDGE_UART_TX = 14;
    static constexpr uint32_t IR_BRIDGE_UART_BAUD = 115200;

    // 外部实时时钟总线引脚。
    static constexpr uint8_t RTC_I2C_SDA = 8;
    static constexpr uint8_t RTC_I2C_SCL = 9;
} // 命名空间结束

#endif
