#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

namespace pins
{
    static constexpr uint8_t DHT_DATA = 13;
    static constexpr uint8_t LIGHT_ANALOG = 4;
    static constexpr uint8_t MQ2_ANALOG = 5;
    static constexpr uint8_t FLAME_ANALOG = 6;

    static constexpr uint8_t FAN_PWM = 18;
    static constexpr uint8_t CURTAIN_SERVO_A = 16;
    static constexpr uint8_t CURTAIN_SERVO_B = 17;
    static constexpr uint8_t BUZZER = 12;

    static constexpr uint8_t IR_BRIDGE_UART_RX = 15;
    static constexpr uint8_t IR_BRIDGE_UART_TX = 14;
    static constexpr uint32_t IR_BRIDGE_UART_BAUD = 115200;

    static constexpr uint8_t RTC_I2C_SDA = 8;
    static constexpr uint8_t RTC_I2C_SCL = 9;
} // namespace pins

#endif
