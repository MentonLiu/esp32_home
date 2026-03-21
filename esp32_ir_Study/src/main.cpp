// 文件说明：esp32_ir_Study/src/main.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}