/**
 * 文件：esp32_home_server/src/main.cpp
 * 功能说明：
 *   - Arduino 应用程序入口点
 *   - 定义 setup() 和 loop() 函数（PlatformIO/Arduino 标准）
 *   - 创建全局 CentralProcessor 实例并驱动系统运行
 *
 * 执行流程：
 *   1. setup() - Arduino 启动时调用一次，初始化 CentralProcessor
 *   2. loop() - Arduino 主循环，高频重复执行，驱动所有状态机
 *
 * 依赖：CentralProcessor.h, Arduino 框架
 * 被依赖于：Arduino/PlatformIO 运行时
 *
 * 设计细节：
 *   - CentralProcessor 是全局单例实例
 *   - loop() 无阻塞设计，支持多个子模块并发运行
 *   - 所有模块通过 CentralProcessor 协调
 *
 * 典型启动时间：
 *   - setup() 耗时：2-5 秒（网络初始化、传感器启动）
 *   - loop() 周期：10-50 毫秒（取决于传感器采样）
 */

#include <Arduino.h>

#include "CentralProcessor.h"

// 应用级总控实例。
CentralProcessor processor;

void setup()
{
    // Arduino 入口：上电后仅执行一次，负责系统初始化。
    processor.begin();
}

void loop()
{
    // Arduino 主循环：高频反复执行，驱动全系统状态机。
    processor.loop();
}
