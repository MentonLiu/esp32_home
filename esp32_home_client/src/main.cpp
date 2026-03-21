// 文件说明：esp32_home_client/src/main.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include <Arduino.h>

#include "HomeClientApp.h"

HomeClientApp app;

void setup()
{
  app.begin();
}

void loop()
{
  app.loop();
}
