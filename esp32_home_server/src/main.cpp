#include <Arduino.h>

#include "CentralProcessor.h"

// 应用级总控实例。
CentralProcessor processor;

void setup()
{
    // 开机时完成所有模块初始化。
    processor.begin();
}

void loop()
{
    // 持续驱动系统状态机运行。
    processor.loop();
}
