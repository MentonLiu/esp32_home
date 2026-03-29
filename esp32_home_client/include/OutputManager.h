// 文件说明：esp32_home_client/include/OutputManager.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef OUTPUT_MANAGER_H
#define OUTPUT_MANAGER_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <TFT_eSPI.h>
#include <Wire.h>

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
    void beginTft();
    void drawTftStaticLayout();
    void renderTftHtmlPage(const ClientWiFiManager &wifiManager,
                           const ServerStatus &status,
                           const String &lastMessage,
                           bool serverReachable);
    void updateRgb(const String &smokeLevel);
    void renderLcd(const ClientWiFiManager &wifiManager,
                   const ServerStatus &status);
    String fit16(const String &text) const;
    String buildClockText() const;
    String buildDateText() const;
    String buildWeekdayText() const;
    String buildConnectionBadge(const ClientWiFiManager &wifiManager) const;
    String buildLiveInfo(const ServerStatus &status) const;
    String buildFooterText(const ServerStatus &status,
                           const String &lastMessage) const;
    uint16_t smokeColor565(const String &smokeLevel) const;

    LiquidCrystal_I2C lcd_;
    TFT_eSPI tft_;
    bool lcdReady_ = false;
    bool tftReady_ = false;
    bool tftStaticPainted_ = false;
    bool firstFrameLogged_ = false;
    unsigned long lastRenderMs_ = 0;
    unsigned long lastRenderLogMs_ = 0;
};

#endif
