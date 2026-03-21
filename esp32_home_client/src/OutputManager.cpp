// 文件说明：esp32_home_client/src/OutputManager.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "OutputManager.h"

#include "ClientConfig.h"

OutputManager::OutputManager()
    : lcd_(client_config::kLcdAddress, client_config::kLcdColumns, client_config::kLcdRows)
{
}

void OutputManager::begin()
{
    pinMode(client_config::kRgbRedPin, OUTPUT);
    pinMode(client_config::kRgbGreenPin, OUTPUT);
    pinMode(client_config::kRgbBluePin, OUTPUT);
    updateRgb("green");

    if (client_config::kEnableLcd1602)
    {
        Wire.begin(client_config::kLcdSda, client_config::kLcdScl);
        lcd_.init();
        lcd_.backlight();
        lcd_.clear();
        lcdReady_ = true;
    }

    // 8Pin 显示器后续将由 LVGL 驱动，当前版本先停用具体绘制实现。
}

void OutputManager::render(const ClientWiFiManager &wifiManager,
                           const ServerStatus &status,
                           ClientControlMode controlMode,
                           const String &lastMessage,
                           bool serverReachable)
{
    updateRgb(status.smokeLevel);

    if (millis() - lastRenderMs_ < client_config::kDisplayRefreshIntervalMs)
    {
        return;
    }
    lastRenderMs_ = millis();

    renderLcd(wifiManager, status, controlMode);
    (void)lastMessage;
    (void)serverReachable;
}

void OutputManager::updateRgb(const String &smokeLevel)
{
    bool red = false;
    bool green = false;
    bool blue = false;

    if (smokeLevel == "green")
    {
        green = true;
    }
    else if (smokeLevel == "blue")
    {
        blue = true;
    }
    else if (smokeLevel == "yellow")
    {
        red = true;
        green = true;
    }
    else
    {
        red = true;
    }

    digitalWrite(client_config::kRgbRedPin, red ? HIGH : LOW);
    digitalWrite(client_config::kRgbGreenPin, green ? HIGH : LOW);
    digitalWrite(client_config::kRgbBluePin, blue ? HIGH : LOW);
}

void OutputManager::renderLcd(const ClientWiFiManager &wifiManager,
                              const ServerStatus &status,
                              ClientControlMode controlMode)
{
    if (!lcdReady_)
    {
        return;
    }

    String line1 = wifiManager.isConnected() ? wifiManager.currentSsid() : "wifi offline";
    if (line1.length() == 0)
    {
        line1 = "wifi waiting";
    }

    const String powerText = status.fanPowerOn ? "ON" : "OFF";
    String line2 = String(controlMode == ClientControlMode::Curtain ? "C" : "F") +
                   " P" + powerText +
                   " " + String(status.fanSpeedPercent) + "%" +
                   " " + status.smokeLevel;

    lcd_.setCursor(0, 0);
    lcd_.print(fit16(line1));
    lcd_.setCursor(0, 1);
    lcd_.print(fit16(line2));
}

String OutputManager::fit16(const String &text) const
{
    String out = text;
    if (out.length() > 16)
    {
        out = out.substring(0, 16);
    }
    while (out.length() < 16)
    {
        out += ' ';
    }
    return out;
}
