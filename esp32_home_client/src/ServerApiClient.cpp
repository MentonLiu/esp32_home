// 文件说明：esp32_home_client/src/ServerApiClient.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "ServerApiClient.h"

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "ClientConfig.h"

ServerApiClient::ServerApiClient(ClientWiFiManager &wifiManager) : wifiManager_(wifiManager) {}

void ServerApiClient::begin()
{
    clearServerCache();
}

bool ServerApiClient::fetchStatus(ServerStatus &status)
{
    if (!wifiManager_.isConnected())
    {
        clearServerCache();
        return false;
    }

    if (!ensureServerResolved())
    {
        serverReachable_ = false;
        return false;
    }

    if (!httpGetStatus(cachedBaseUrl_, status))
    {
        clearServerCache();
        return false;
    }

    serverReachable_ = true;
    return true;
}

ControlResponse ServerApiClient::sendCommand(const String &jsonPayload)
{
    ControlResponse response;
    if (!wifiManager_.isConnected() || !ensureServerResolved())
    {
        response.message = "server_unreachable";
        return response;
    }

    HTTPClient http;
    http.setTimeout(client_config::kHttpTimeoutMs);
    if (!http.begin(cachedBaseUrl_ + "/api/control"))
    {
        response.message = "http_begin_failed";
        clearServerCache();
        return response;
    }

    http.addHeader("Content-Type", "application/json");
    response.httpCode = http.POST(jsonPayload);
    const String payload = http.getString();
    http.end();

    if (response.httpCode <= 0)
    {
        response.message = "http_post_failed";
        clearServerCache();
        return response;
    }

    JsonDocument doc;
    if (deserializeJson(doc, payload))
    {
        response.ok = response.httpCode >= 200 && response.httpCode < 300;
        response.message = response.ok ? "ok" : "invalid_json_response";
        return response;
    }

    response.ok = doc["ok"] | false;
    response.stateChanged = doc["stateChanged"] | false;
    response.type = String(doc["type"] | "");
    response.message = String(doc["message"] | "");
    serverReachable_ = true;
    return response;
}

bool ServerApiClient::isServerReachable() const
{
    return serverReachable_;
}

String ServerApiClient::serverBaseUrl() const
{
    return cachedBaseUrl_;
}

bool ServerApiClient::ensureMdnsReady()
{
    if (!wifiManager_.isConnected())
    {
        mdnsReady_ = false;
        return false;
    }

    if (mdnsReady_)
    {
        return true;
    }

    mdnsReady_ = MDNS.begin(client_config::kClientMdnsName);
    return mdnsReady_;
}

bool ServerApiClient::ensureServerResolved()
{
    if (!wifiManager_.isConnected())
    {
        clearServerCache();
        return false;
    }

    if (cachedBaseUrl_.length() > 0)
    {
        return true;
    }

    if (millis() - lastDiscoveryMs_ < client_config::kServerRediscoveryIntervalMs)
    {
        return false;
    }
    lastDiscoveryMs_ = millis();

    if (wifiManager_.isConnectedToServerAp())
    {
        return probeAndCache(client_config::kServerApIp);
    }

    ensureMdnsReady();
    const IPAddress mdnsIp = MDNS.queryHost(client_config::kServerMdnsName, 1000);
    if (mdnsIp != INADDR_NONE && probeAndCache(mdnsIp.toString()))
    {
        return true;
    }

    if (cachedHost_.length() > 0)
    {
        return probeAndCache(cachedHost_);
    }

    return false;
}

bool ServerApiClient::probeAndCache(const String &host)
{
    if (host.length() == 0)
    {
        return false;
    }

    ServerStatus status;
    const String baseUrl = buildBaseUrl(host);
    if (!httpGetStatus(baseUrl, status))
    {
        return false;
    }

    cachedHost_ = host;
    cachedBaseUrl_ = baseUrl;
    serverReachable_ = true;
    return true;
}

bool ServerApiClient::httpGetStatus(const String &baseUrl, ServerStatus &status)
{
    HTTPClient http;
    http.setTimeout(client_config::kHttpTimeoutMs);
    if (!http.begin(baseUrl + "/api/status"))
    {
        return false;
    }

    const int httpCode = http.GET();
    const String payload = http.getString();
    http.end();

    if (httpCode != HTTP_CODE_OK)
    {
        return false;
    }

    return parseStatusPayload(payload, baseUrl, status);
}

bool ServerApiClient::parseStatusPayload(const String &payload, const String &baseUrl, ServerStatus &status) const
{
    JsonDocument doc;
    if (deserializeJson(doc, payload))
    {
        return false;
    }

    status.online = true;
    status.serverBaseUrl = baseUrl;
    status.mode = String(doc["mode"] | doc["system"]["mode"] | "unknown");
    status.ip = String(doc["ip"] | doc["system"]["ip"] | "");
    status.temperatureC = doc["temperatureC"] | doc["sensor"]["temperatureC"] | 0.0F;
    status.humidityPercent = doc["humidityPercent"] | doc["sensor"]["humidityPercent"] | 0.0F;
    status.lightPercent = doc["lightPercent"] | doc["sensor"]["lightPercent"] | 0;
    status.mq2Percent = doc["mq2Percent"] | doc["sensor"]["mq2Percent"] | 0;
    status.smokeLevel = String(doc["smokeLevel"] | doc["sensor"]["smokeLevel"] | "green");
    status.flameDetected = doc["flameDetected"] | doc["sensor"]["flameDetected"] | false;
    status.fanMode = String(doc["fanMode"] | doc["controller"]["fanMode"] | "off");
    status.fanSpeedPercent = doc["fanSpeedPercent"] | doc["controller"]["fanSpeedPercent"] | 0;
    status.curtainAngle = doc["curtainAngle"] | doc["controller"]["curtainAngle"] | 0;
    status.error = doc["error"] | doc["sensor"]["error"] | false;
    status.errorMessage = String(doc["errorMessage"] | doc["sensor"]["errorMessage"] | "");
    status.sensorTimestamp = doc["sensorTimestamp"] | doc["sensor"]["timestamp"] | 0UL;
    return true;
}

String ServerApiClient::buildBaseUrl(const String &host) const
{
    return String("http://") + host;
}

void ServerApiClient::clearServerCache()
{
    cachedBaseUrl_ = "";
    serverReachable_ = false;
}
