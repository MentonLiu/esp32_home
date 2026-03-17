/**
 * @file pins.h
 * @brief 引脚定义文件
 *
 * 定义ESP32开发板所有使用的GPIO引脚，包括传感器、执行器和通信模块
 * 采用namespace封装，避免全局变量污染
 */

#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

namespace pins
{
    /**
     * @brief DHT系列温湿度传感器引脚定义
     * DHT22: 4线传感器，支持更宽的温度范围和更高精度
     * DHT11_DATA_COMPAT: 兼容DHT11 3针模块的引脚
     */
    static constexpr uint8_t DHT_DATA = 13;
    static constexpr uint8_t DHT11_DATA_COMPAT = DHT_DATA;

    /**
     * @brief 模拟传感器引脚定义
     * LDR: 光敏电阻，检测环境光照强度
     * MQ2: 烟雾/燃气传感器，检测空气中可燃气体浓度
     */
    static constexpr uint8_t LDR_ANALOG = 34;
    static constexpr uint8_t MQ2_ANALOG = 36;

    /**
     * @brief 火焰传感器引脚
     * 复用GPIO39作为火焰强度输入（无专用模块时作为占位符）
     * 当检测到火焰时，模拟值会升高
     */
    static constexpr uint8_t FLAME_ANALOG = 39;

    /**
     * @brief 继电器控制风扇引脚
     * 通过PWM控制风扇转速
     */
    static constexpr uint8_t RELAY_FAN = 18;

    /**
     * @brief 窗帘舵机控制引脚
     * 双舵机配置：一个控制左侧，一个控制右侧
     * 舵机角度范围：0-180度
     */
    static constexpr uint8_t SERVO_1 = 19;
    static constexpr uint8_t SERVO_2 = 21;
    static constexpr uint8_t SERVO_3 = 22;
    static constexpr uint8_t SERVO_4 = 14;

    /**
     * @brief 蜂鸣器引脚
     * 用于报警和提示音输出
     */
    static constexpr uint8_t BUZZER = 12;

    /**
     * @brief ESP8266红外桥接串口引脚
     * @details
     * ESP32通过UART2向外接ESP8266模块发送红外控制命令，
     * ESP8266负责真正的红外收发工作。
     */
    static constexpr uint8_t IR_BRIDGE_UART_RX = 16;
    static constexpr uint8_t IR_BRIDGE_UART_TX = 17;
    static constexpr uint32_t IR_BRIDGE_UART_BAUD = 115200;

} // namespace pins

#endif
