// 文件说明：esp32_home_server/src/main.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

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
