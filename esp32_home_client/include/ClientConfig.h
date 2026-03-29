// 文件说明：esp32_home_client/include/ClientConfig.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef CLIENT_CONFIG_H
#define CLIENT_CONFIG_H

#include <Arduino.h>

namespace client_config
{
    // 已知的目标网络：优先家庭路由器，其次服务端兜底热点。
    constexpr const char *kPrimaryWifiSsid = "HW-2103";
    constexpr const char *kPrimaryWifiPassword = "20220715";
    constexpr const char *kServerApSsid = "esp32-server";
    constexpr const char *kServerApPassword = "lbl450981";

    // 服务端在热点模式下的固定地址，以及云/局域网模式下的 mDNS 名称。
    constexpr const char *kServerApIp = "192.168.4.1";
    constexpr const char *kServerMdnsName = "esp32-home-server";
    constexpr const char *kClientMdnsName = "esp32-home-client";

    constexpr unsigned long kWifiRetryIntervalMs = 12000UL;
    constexpr unsigned long kWifiConnectTimeoutMs = 8000UL;
    constexpr unsigned long kStatusPollIntervalMs = 800UL;
    constexpr unsigned long kDisplayRefreshIntervalMs = 250UL;
    constexpr unsigned long kControlCooldownMs = 180UL;
    constexpr unsigned long kServerRediscoveryIntervalMs = 3000UL;
    constexpr unsigned long kHttpTimeoutMs = 1200UL;

    // 诊断日志配置：级别 1=E, 2=W, 3=I, 4=D。
    constexpr bool kEnableDiagnosticsLog = true;
    constexpr uint8_t kDiagnosticsLogLevel = 3;
    constexpr unsigned long kDiagnosticsPeriodicLogMs = 2000UL;

    // 8Pin 显示器切到 LVGL 驱动，当前先完成底层初始化与留白显示。
    constexpr bool kEnableTft = true;
    constexpr bool kEnableLcd1602 = true;

    // TFT SPI 引脚（240x320 SPI 屏幕）。
    constexpr uint8_t kTftSclk = 18;
    constexpr uint8_t kTftMosi = 23;
    constexpr int8_t kTftMiso = -1;
    constexpr uint8_t kTftRst = 17;
    constexpr uint8_t kTftDc = 16;
    constexpr uint8_t kTftCs = 27;
    constexpr uint8_t kTftBacklight = 19;
    constexpr uint16_t kTftWidth = 240;
    constexpr uint16_t kTftHeight = 320;

    // LCD1602 I2C 参数。
    constexpr uint8_t kLcdSda = 21;
    constexpr uint8_t kLcdScl = 22;
    constexpr uint8_t kLcdAddress = 0x27;
    constexpr uint8_t kLcdColumns = 16;
    constexpr uint8_t kLcdRows = 2;

    // RGB 指示灯。
    constexpr uint8_t kRgbRedPin = 25;
    constexpr uint8_t kRgbGreenPin = 14;
    constexpr uint8_t kRgbBluePin = 15;

    // 四个独立按钮：按钮 1/2 控制窗帘和风扇状态，按钮 3/4 预留。
    constexpr uint8_t kButton1Pin = 32;
    constexpr uint8_t kButton2Pin = 33;
    constexpr uint8_t kButton3Pin = 26;
    constexpr uint8_t kButton4Pin = 13;

} // namespace client_config

#endif
