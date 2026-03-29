// 文件说明：esp32_home_client/src/ServerApiClient.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "ServerApiClient.h"

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "ClientConfig.h"
#include "ClientLog.h"

ServerApiClient::ServerApiClient(ClientWiFiManager &wifiManager) : wifiManager_(wifiManager) {}

void ServerApiClient::begin()
{
    CL_INFO("API", "begin", "phase=start");
    clearServerCache();
    CL_INFO("API", "begin", "phase=done");
}

bool ServerApiClient::fetchStatus(ServerStatus &status)
{
    const unsigned long startMs = millis();
    if (!wifiManager_.isConnected())
    {
        clearServerCache();
        CL_WARN("API", "fetch_skip", "reason=wifi_disconnected");
        return false;
    }

    if (!ensureServerResolved())
    {
        serverReachable_ = false;
        CL_WARN("API", "fetch_fail", "reason=server_unresolved");
        return false;
    }

    if (!httpGetStatus(cachedBaseUrl_, status))
    {
        clearServerCache();
        CL_WARN("API", "fetch_fail", "reason=http_get_failed");
        return false;
    }

    serverReachable_ = true;
    CL_INFOF("API", "fetch_ok", "dur_ms=%lu base=%s", millis() - startMs, cachedBaseUrl_.c_str());
    return true;
}

ControlResponse ServerApiClient::sendCommand(const String &jsonPayload)
{
    ControlResponse response;
    const unsigned long startMs = millis();
    if (!wifiManager_.isConnected() || !ensureServerResolved())
    {
        response.message = "server_unreachable";
        CL_WARN("API", "control_skip", "reason=server_unreachable");
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
        CL_ERRORF("API", "control_http_fail", "code=%d dur_ms=%lu", response.httpCode, millis() - startMs);
        return response;
    }

    JsonDocument doc;
    if (deserializeJson(doc, payload))
    {
        response.ok = response.httpCode >= 200 && response.httpCode < 300;
        response.message = response.ok ? "ok" : "invalid_json_response";
        CL_WARNF("API", "control_parse_fallback", "code=%d ok=%d", response.httpCode, response.ok ? 1 : 0);
        return response;
    }

    response.ok = doc["ok"] | false;
    response.stateChanged = doc["stateChanged"] | false;
    response.type = String(doc["type"] | "");
    response.message = String(doc["message"] | "");
    serverReachable_ = true;
    CL_INFOF("API", "control_ok", "code=%d ok=%d state_changed=%d dur_ms=%lu", response.httpCode, response.ok ? 1 : 0, response.stateChanged ? 1 : 0, millis() - startMs);
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
    CL_INFOF("API", "mdns_ready", "ok=%d host=%s", mdnsReady_ ? 1 : 0, client_config::kClientMdnsName);
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
        CL_DEBUGF("API", "discover_cached", "base=%s", cachedBaseUrl_.c_str());
        return true;
    }

    const unsigned long elapsedMs = millis() - lastDiscoveryMs_;
    if (elapsedMs < client_config::kServerRediscoveryIntervalMs)
    {
        static unsigned long lastDiscoverSkipLogMs = 0;
        if (client_log::allowPeriodic(lastDiscoverSkipLogMs, client_config::kDiagnosticsPeriodicLogMs))
        {
            CL_WARNF("API", "discover_skip", "remaining_ms=%lu", client_config::kServerRediscoveryIntervalMs - elapsedMs);
        }
        return false;
    }
    lastDiscoveryMs_ = millis();

    if (wifiManager_.isConnectedToServerAp())
    {
        CL_INFOF("API", "discover_try", "mode=ap ip=%s", client_config::kServerApIp);
        return probeAndCache(client_config::kServerApIp);
    }

    ensureMdnsReady();
    const unsigned long mdnsStartMs = millis();
    const IPAddress mdnsIp = MDNS.queryHost(client_config::kServerMdnsName, 1000);
    CL_INFOF("API", "discover_mdns", "dur_ms=%lu found=%d", millis() - mdnsStartMs, mdnsIp != INADDR_NONE ? 1 : 0);
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
    CL_INFOF("API", "probe", "host=%s", host.c_str());
    if (!httpGetStatus(baseUrl, status))
    {
        CL_WARNF("API", "probe_fail", "host=%s", host.c_str());
        return false;
    }

    cachedHost_ = host;
    cachedBaseUrl_ = baseUrl;
    serverReachable_ = true;
    CL_INFOF("API", "probe_ok", "base=%s", cachedBaseUrl_.c_str());
    return true;
}

bool ServerApiClient::httpGetStatus(const String &baseUrl, ServerStatus &status)
{
    const unsigned long startMs = millis();
    HTTPClient http;
    http.setTimeout(client_config::kHttpTimeoutMs);
    if (!http.begin(baseUrl + "/api/status"))
    {
        CL_ERRORF("API", "http_begin_fail", "base=%s", baseUrl.c_str());
        return false;
    }

    const int httpCode = http.GET();
    const String payload = http.getString();
    http.end();

    if (httpCode != HTTP_CODE_OK)
    {
        CL_WARNF("API", "http_get_fail", "base=%s code=%d dur_ms=%lu", baseUrl.c_str(), httpCode, millis() - startMs);
        return false;
    }

    const bool parsed = parseStatusPayload(payload, baseUrl, status);
    if (!parsed)
    {
        CL_WARNF("API", "http_parse_fail", "base=%s dur_ms=%lu", baseUrl.c_str(), millis() - startMs);
        return false;
    }

    CL_INFOF("API", "http_get_ok", "base=%s dur_ms=%lu", baseUrl.c_str(), millis() - startMs);
    return true;
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
    const bool hadCache = cachedBaseUrl_.length() > 0 || cachedHost_.length() > 0 || serverReachable_;
    cachedBaseUrl_ = "";
    cachedHost_ = "";
    serverReachable_ = false;
    if (hadCache)
    {
        CL_WARN("API", "cache_clear", "reason=reset_or_error");
    }
}
