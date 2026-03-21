// 文件说明：esp32_home_client/include/OutputManager.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef OUTPUT_MANAGER_H
#define OUTPUT_MANAGER_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#include "ClientContracts.h"
#include "ClientWiFiManager.h"

class OutputManager
{
public:
    OutputManager();

    void begin();
    void render(const ClientWiFiManager &wifiManager,
                const ServerStatus &status,
                ClientControlMode controlMode,
                const String &lastMessage,
                bool serverReachable);

private:
    void updateRgb(const String &smokeLevel);
    void renderLcd(const ClientWiFiManager &wifiManager,
                   const ServerStatus &status,
                   ClientControlMode controlMode);
    String fit16(const String &text) const;

    LiquidCrystal_I2C lcd_;
    bool lcdReady_ = false;
    unsigned long lastRenderMs_ = 0;
};

#endif
