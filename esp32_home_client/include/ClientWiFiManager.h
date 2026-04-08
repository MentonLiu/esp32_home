// 文件说明：esp32_home_client/include/ClientWiFiManager.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef CLIENT_WIFI_MANAGER_H
#define CLIENT_WIFI_MANAGER_H

#include <Arduino.h>

class ClientWiFiManager
{
public:
    // 初始化 STA 模式并触发首次连接。
    void begin();
    // 维持连接并在断线时重试。
    void loop();

    // 当前站点是否已联网并拿到 IP。
    bool isConnected() const;
    // 当前是否连接到服务端兜底热点。
    bool isConnectedToServerAp() const;
    // 返回当前 SSID，离线时为空字符串。
    String currentSsid() const;
    // 返回本机 IP 文本，离线时为 0.0.0.0。
    String localIpString() const;

private:
    // 先尝试主 Wi-Fi，再尝试兜底热点。
    bool connectKnownNetworks();
    // 连接指定 SSID，并受超时约束。
    bool connectTo(const char *ssid, const char *password);

    // 最近一次连接尝试的时间戳。
    unsigned long lastAttemptMs_ = 0;
};

#endif
