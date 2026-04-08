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

    // 重置发现与缓存状态。
    void begin();
    // 拉取 /api/status 并填充状态模型。
    bool fetchStatus(ServerStatus &status);
    // 发送 /api/control 的 JSON 负载。
    ControlResponse sendCommand(const String &jsonPayload);

    // 最近是否可达服务端。
    bool isServerReachable() const;
    // 返回已发现并缓存的基础 URL。
    String serverBaseUrl() const;

private:
    // 为当前客户端主机启动 mDNS。
    bool ensureMdnsReady();
    // 解析并缓存可达的服务端地址。
    bool ensureServerResolved();
    // 探测候选主机并更新缓存。
    bool probeAndCache(const String &host);
    // 执行 HTTP GET /api/status。
    bool httpGetStatus(const String &baseUrl, ServerStatus &status);
    // 将 JSON 状态负载解析为 ServerStatus。
    bool parseStatusPayload(const String &payload, const String &baseUrl, ServerStatus &status) const;
    // 组装带协议前缀的基础 URL。
    String buildBaseUrl(const String &host) const;
    // 清空发现缓存与可达状态。
    void clearServerCache();

    // 共享的 Wi-Fi 状态提供者。
    ClientWiFiManager &wifiManager_;
    bool mdnsReady_ = false;
    String cachedHost_;
    String cachedBaseUrl_;
    bool serverReachable_ = false;
    unsigned long lastDiscoveryMs_ = 0;
};

#endif
