// 文件说明：esp32_home_client/include/ClientConfig.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef CLIENT_CONFIG_H
#define CLIENT_CONFIG_H

#include <Arduino.h>

namespace client_config
{
    // 已知的目标网络：优先家庭路由器，其次服务端兜底热点。
    constexpr const char *kPrimaryWifiSsid = "iPadmini";
    constexpr const char *kPrimaryWifiPassword = "lbl450981";
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

    constexpr uint8_t kCurtainStepDegrees = 5;
    constexpr uint8_t kFanStepPercent = 5;

    // 8Pin 显示器后续会切到 LVGL，目前先屏蔽显示实现。
    constexpr bool kEnableTft = false;
    constexpr bool kEnableLcd1602 = true;

    // TFT SPI 引脚（按常见 ST7789 8Pin 接法预设，可按实际接线调整）。
    constexpr uint8_t kTftSclk = 18;
    constexpr uint8_t kTftMosi = 23;
    constexpr int8_t kTftMiso = -1;
    constexpr uint8_t kTftCs = 5;
    constexpr uint8_t kTftDc = 17;
    constexpr uint8_t kTftRst = 16;
    constexpr uint8_t kTftBacklight = 4;

    // LCD1602 I2C 参数。
    constexpr uint8_t kLcdSda = 21;
    constexpr uint8_t kLcdScl = 22;
    constexpr uint8_t kLcdAddress = 0x27;
    constexpr uint8_t kLcdColumns = 16;
    constexpr uint8_t kLcdRows = 2;

    // RGB 指示灯。
    constexpr uint8_t kRgbRedPin = 27;
    constexpr uint8_t kRgbGreenPin = 14;
    constexpr uint8_t kRgbBluePin = 15;

    constexpr uint8_t kFanPowerButtonPin = 19;

    // 旋钮编码器 A/B 与按键。
    constexpr uint8_t kEncoderPinA = 34;
    constexpr uint8_t kEncoderPinB = 35;
    constexpr uint8_t kEncoderButtonPin = 13;

} // namespace client_config

#endif
