// 文件说明：esp32_home_client/include/OutputManager.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef OUTPUT_MANAGER_H
#define OUTPUT_MANAGER_H

#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>
#include <time.h>

#include "ClientConfig.h"
#include "ClientContracts.h"
#include "ClientWiFiManager.h"

class OutputManager
{
public:
    OutputManager();

    // 初始化 RGB 与双 OLED 硬件。
    void begin();
    // 渲染当前状态的一帧画面。
    void render(const ClientWiFiManager &wifiManager,
                const ServerStatus &status,
                const String &lastMessage,
                bool serverReachable);

private:
    // 初始化双 OLED，并分别挂到独立 I2C 总线。
    void beginOleds();
    // 左侧 OLED 显示环境传感器数据。
    void renderSensorOled(const ServerStatus &status);
    // 右侧 OLED 显示控制状态、网络与时间。
    void renderStatusOled(const ClientWiFiManager &wifiManager,
                          const ServerStatus &status,
                          const String &lastMessage,
                          bool serverReachable);
    // 将烟雾等级映射到 RGB 指示灯。
    void updateRgb(const String &smokeLevel);
    // 配置并重试 NTP 时间同步。
    void syncSystemTime(const ClientWiFiManager &wifiManager);
    // 获取校验后的本地时间快照。
    bool getCurrentLocalTime(struct tm &timeinfo) const;
    String buildClockText() const;
    String buildDateText() const;
    String buildWeekdayText() const;
    String buildConnectionBadge(const ClientWiFiManager &wifiManager) const;
    String buildFooterText(const ServerStatus &status,
                           const String &lastMessage) const;
    String buildSmokeLabel(const String &smokeLevel) const;
    String fitLine(const String &text, size_t maxLength) const;

    TwoWire sensorOledWire_;
    TwoWire statusOledWire_;
    Adafruit_SSD1306 sensorOled_;
    Adafruit_SSD1306 statusOled_;
    bool sensorOledReady_ = false;
    bool statusOledReady_ = false;
    bool firstFrameLogged_ = false;
    bool timeConfigApplied_ = false;
    bool timeSynced_ = false;
    unsigned long lastRenderMs_ = 0;
    unsigned long lastRenderLogMs_ = 0;
    unsigned long lastTimeSyncAttemptMs_ = 0;
};

#endif
