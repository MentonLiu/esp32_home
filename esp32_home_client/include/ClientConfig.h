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
    constexpr unsigned long kTimeSyncRetryIntervalMs = 15000UL;

    // 时间同步配置：默认使用中国时区，如需其他地区可调整 POSIX TZ 字符串。
    constexpr const char *kTimeZone = "CST-8";
    constexpr const char *kNtpServerPrimary = "ntp.aliyun.com";
    constexpr const char *kNtpServerSecondary = "pool.ntp.org";

    // 诊断日志配置：级别 1=E, 2=W, 3=I, 4=D。
    constexpr bool kEnableDiagnosticsLog = true;
    constexpr uint8_t kDiagnosticsLogLevel = 3;
    constexpr unsigned long kDiagnosticsPeriodicLogMs = 2000UL;

    // 原 TFT/LCD 显示方案已停用，改为双四针 I2C OLED。
    constexpr bool kEnableSensorOled = true;
    constexpr bool kEnableStatusOled = true;

    // 双 OLED 分别使用独立 I2C 总线，避免地址冲突。
    constexpr uint8_t kSensorOledSda = 21;
    constexpr uint8_t kSensorOledScl = 22;
    constexpr uint8_t kStatusOledSda = 23;
    constexpr uint8_t kStatusOledScl = 18;
    constexpr uint8_t kSensorOledAddress = 0x3C;
    constexpr uint8_t kStatusOledAddress = 0x3C;
    constexpr int8_t kOledResetPin = -1;
    constexpr uint8_t kOledWidth = 128;
    constexpr uint8_t kOledHeight = 64;

    // RGB 指示灯。
    constexpr uint8_t kRgbRedPin = 25;
    constexpr uint8_t kRgbGreenPin = 14;
    constexpr uint8_t kRgbBluePin = 15;

    // 四个独立按钮：按钮 1/2 控制窗帘和风扇状态，按钮 3/4 预留。
    constexpr uint8_t kButton1Pin = 32; // 窗帘状态切换
    constexpr uint8_t kButton2Pin = 33; // 窗帘/风扇状态切换
    constexpr uint8_t kButton3Pin = 26; // 预留
    constexpr uint8_t kButton4Pin = 13; // 预留

} // namespace client_config

#endif
