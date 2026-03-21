// 文件说明：esp32_home_client/include/ServerApiClient.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef SERVER_API_CLIENT_H
#define SERVER_API_CLIENT_H

#include <Arduino.h>

#include "ClientContracts.h"
#include "ClientWiFiManager.h"

class ServerApiClient
{
public:
    explicit ServerApiClient(ClientWiFiManager &wifiManager);

    void begin();
    bool fetchStatus(ServerStatus &status);
    ControlResponse sendCommand(const String &jsonPayload);

    bool isServerReachable() const;
    String serverBaseUrl() const;

private:
    bool ensureMdnsReady();
    bool ensureServerResolved();
    bool probeAndCache(const String &host);
    bool httpGetStatus(const String &baseUrl, ServerStatus &status);
    bool parseStatusPayload(const String &payload, const String &baseUrl, ServerStatus &status) const;
    String buildBaseUrl(const String &host) const;
    void clearServerCache();

    ClientWiFiManager &wifiManager_;
    bool mdnsReady_ = false;
    String cachedHost_;
    String cachedBaseUrl_;
    bool serverReachable_ = false;
    unsigned long lastDiscoveryMs_ = 0;
};

#endif
