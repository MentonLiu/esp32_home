// 文件说明：esp32_home_client/src/main.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include <Arduino.h>
#include <esp_system.h>

#include "ClientLog.h"
#include "HomeClientApp.h"

HomeClientApp app;

namespace
{
  const char *resetReasonToText(esp_reset_reason_t reason)
  {
    switch (reason)
    {
    case ESP_RST_POWERON:
      return "POWERON";
    case ESP_RST_EXT:
      return "EXT";
    case ESP_RST_SW:
      return "SW";
    case ESP_RST_PANIC:
      return "PANIC";
    case ESP_RST_INT_WDT:
      return "INT_WDT";
    case ESP_RST_TASK_WDT:
      return "TASK_WDT";
    case ESP_RST_WDT:
      return "WDT";
    case ESP_RST_DEEPSLEEP:
      return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:
      return "BROWNOUT";
    case ESP_RST_SDIO:
      return "SDIO";
    default:
      return "UNKNOWN";
    }
  }
} // namespace

void setup()
{
  client_log::markBootStart();
  CL_INFO("BOOT", "setup_begin", "phase=entry");
  const esp_reset_reason_t reason = esp_reset_reason();
  CL_INFOF("BOOT", "reset_reason", "reason=%s code=%d", resetReasonToText(reason), static_cast<int>(reason));

  app.begin();
  CL_INFO("BOOT", "setup_done", "phase=app_begin_complete");
}

void loop()
{
  app.loop();
}
