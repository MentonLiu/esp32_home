// 文件说明：esp32_home_client/include/ClientWiFiManager.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef CLIENT_WIFI_MANAGER_H
#define CLIENT_WIFI_MANAGER_H

#include <Arduino.h>

class ClientWiFiManager
{
public:
    void begin();
    void loop();

    bool isConnected() const;
    bool isConnectedToServerAp() const;
    String currentSsid() const;
    String localIpString() const;

private:
    bool connectKnownNetworks();
    bool connectTo(const char *ssid, const char *password);

    unsigned long lastAttemptMs_ = 0;
};

#endif
