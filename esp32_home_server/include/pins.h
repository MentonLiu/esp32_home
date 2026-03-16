#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

namespace pins
{
    // DHT22 4-pin wiring, while keeping compatibility pin for DHT11 3-pin modules.
    static constexpr uint8_t DHT_DATA = 13;
    static constexpr uint8_t DHT11_DATA_COMPAT = DHT_DATA;

    static constexpr uint8_t LDR_ANALOG = 34;
    static constexpr uint8_t MQ2_ANALOG = 36;

    // Diagram does not include a dedicated flame module pin.
    // Reuse analog input from pin 39 as a flame intensity input placeholder.
    static constexpr uint8_t FLAME_ANALOG = 39;

    static constexpr uint8_t RELAY_FAN = 18;

    static constexpr uint8_t SERVO_1 = 19;
    static constexpr uint8_t SERVO_2 = 21;
    static constexpr uint8_t SERVO_3 = 22;
    static constexpr uint8_t SERVO_4 = 14;

    static constexpr uint8_t BUZZER = 12;

    static constexpr uint8_t IR_RX = 5;
    static constexpr uint8_t IR_TX = 27;
} // namespace pins

#endif