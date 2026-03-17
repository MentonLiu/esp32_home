/**
 * @file main.cpp
 * @brief ESP32智能家居系统主入口文件
 *
 * 程序入口点，初始化HomeService并进入主循环
 * 整个系统功能由HomeService类封装
 */

#include <Arduino.h>

#include "HomeService.h"

/**
 * @brief 全局服务实例
 * 所有业务逻辑都在HomeService类中处理
 */
HomeService homeService;

/**
 * @brief 系统初始化函数
 * Arduino框架启动时调用一次
 */
void setup()
{
    homeService.begin();
}

/**
 * @brief 主循环函数
 * Arduino框架持续调用，处理所有业务逻辑
 */
void loop()
{
    homeService.loop();
}
