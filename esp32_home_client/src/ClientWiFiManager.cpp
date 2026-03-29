// 文件说明：esp32_home_client/src/ClientWiFiManager.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "ClientWiFiManager.h"

#include <WiFi.h>

#include "ClientConfig.h"
#include "ClientLog.h"

void ClientWiFiManager::begin()
{
    CL_INFO("WIFI", "begin", "mode=sta");
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    connectKnownNetworks();
}

void ClientWiFiManager::loop()
{
    const bool connected = isConnected();
    static bool lastConnectedState = false;
    static bool stateInitialized = false;
    if (!stateInitialized)
    {
        lastConnectedState = connected;
        stateInitialized = true;
    }

    if (client_log::changedBool(connected, lastConnectedState))
    {
        CL_INFOF("WIFI", "state_change", "connected=%d ssid=%s ip=%s", connected ? 1 : 0, currentSsid().c_str(), localIpString().c_str());
    }

    if (connected)
    {
        return;
    }

    const unsigned long elapsedMs = millis() - lastAttemptMs_;
    if (elapsedMs < client_config::kWifiRetryIntervalMs)
    {
        static unsigned long lastRetryLogMs = 0;
        if (client_log::allowPeriodic(lastRetryLogMs, client_config::kDiagnosticsPeriodicLogMs))
        {
            CL_WARNF("WIFI", "retry_wait", "remaining_ms=%lu", client_config::kWifiRetryIntervalMs - elapsedMs);
        }
        return;
    }

    CL_INFO("WIFI", "retry_now", "reason=disconnected");
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
    CL_INFO("WIFI", "connect_known", "phase=start");

    if (connectTo(client_config::kPrimaryWifiSsid, client_config::kPrimaryWifiPassword))
    {
        CL_INFO("WIFI", "connect_known", "result=primary_ok");
        return true;
    }

    const bool fallbackOk = connectTo(client_config::kServerApSsid, client_config::kServerApPassword);
    CL_INFOF("WIFI", "connect_known", "result=%s", fallbackOk ? "server_ap_ok" : "all_failed");
    return fallbackOk;
}

bool ClientWiFiManager::connectTo(const char *ssid, const char *password)
{
    if (ssid == nullptr || strlen(ssid) == 0)
    {
        return false;
    }

    WiFi.disconnect(false, true);
    delay(100);
    CL_INFOF("WIFI", "connect_try", "ssid=%s timeout_ms=%lu", ssid, client_config::kWifiConnectTimeoutMs);
    WiFi.begin(ssid, password);

    const unsigned long startMs = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startMs < client_config::kWifiConnectTimeoutMs)
    {
        delay(200);
    }

    const bool ok = WiFi.status() == WL_CONNECTED;
    if (ok)
    {
        CL_INFOF("WIFI", "connect_ok", "ssid=%s ip=%s dur_ms=%lu", ssid, WiFi.localIP().toString().c_str(), millis() - startMs);
    }
    else
    {
        CL_WARNF("WIFI", "connect_fail", "ssid=%s dur_ms=%lu", ssid, millis() - startMs);
    }
    return ok;
}
