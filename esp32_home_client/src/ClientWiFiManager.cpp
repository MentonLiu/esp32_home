// 文件说明：esp32_home_client/src/ClientWiFiManager.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "ClientWiFiManager.h"

#include <WiFi.h>

#include "ClientConfig.h"

void ClientWiFiManager::begin()
{
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    connectKnownNetworks();
}

void ClientWiFiManager::loop()
{
    if (isConnected())
    {
        return;
    }

    if (millis() - lastAttemptMs_ < client_config::kWifiRetryIntervalMs)
    {
        return;
    }

    connectKnownNetworks();
}

bool ClientWiFiManager::isConnected() const
{
    return WiFi.status() == WL_CONNECTED;
}

bool ClientWiFiManager::isConnectedToServerAp() const
{
    return isConnected() && currentSsid() == client_config::kServerApSsid;
}

String ClientWiFiManager::currentSsid() const
{
    return isConnected() ? WiFi.SSID() : "";
}

String ClientWiFiManager::localIpString() const
{
    return isConnected() ? WiFi.localIP().toString() : "0.0.0.0";
}

bool ClientWiFiManager::connectKnownNetworks()
{
    lastAttemptMs_ = millis();

    if (connectTo(client_config::kPrimaryWifiSsid, client_config::kPrimaryWifiPassword))
    {
        return true;
    }

    return connectTo(client_config::kServerApSsid, client_config::kServerApPassword);
}

bool ClientWiFiManager::connectTo(const char *ssid, const char *password)
{
    if (ssid == nullptr || strlen(ssid) == 0)
    {
        return false;
    }

    WiFi.disconnect(false, true);
    delay(100);
    WiFi.begin(ssid, password);

    const unsigned long startMs = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startMs < client_config::kWifiConnectTimeoutMs)
    {
        delay(200);
    }

    return WiFi.status() == WL_CONNECTED;
}
