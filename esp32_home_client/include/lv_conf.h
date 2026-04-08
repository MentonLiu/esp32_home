#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

// 本项目最小化 LVGL 编译期配置。
// 保持 LVGL 为 RGB565，以匹配当前 ESP32 TFT 渲染链路。
#define LV_COLOR_DEPTH 16

#endif
