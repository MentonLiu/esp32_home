// 文件说明：esp32_home_client/include/OutputManager.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef OUTPUT_MANAGER_H
#define OUTPUT_MANAGER_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <lvgl.h>

#include "ClientConfig.h"
#include "ClientContracts.h"
#include "ClientWiFiManager.h"

class OutputManager
{
public:
    OutputManager();

    void begin();
    void render(const ClientWiFiManager &wifiManager,
                const ServerStatus &status,
                const String &lastMessage,
                bool serverReachable);

private:
    static void flushDisplay(lv_disp_drv_t *dispDriver, const lv_area_t *area, lv_color_t *colorMap);
    void beginTft();
    void renderLvgl();
    void updateRgb(const String &smokeLevel);
    void renderLcd(const ClientWiFiManager &wifiManager,
                   const ServerStatus &status);
    String fit16(const String &text) const;

    LiquidCrystal_I2C lcd_;
    TFT_eSPI tft_;
    lv_disp_draw_buf_t lvDrawBuf_{};
    lv_disp_drv_t lvDispDrv_{};
    lv_color_t lvDrawBuffer_[client_config::kTftWidth * 20];
    bool lcdReady_ = false;
    bool tftReady_ = false;
    unsigned long lastRenderMs_ = 0;
    unsigned long lastLvTickMs_ = 0;
};

#endif
