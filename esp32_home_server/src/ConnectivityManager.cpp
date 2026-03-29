// 文件说明：esp32_home_server/src/ConnectivityManager.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "ConnectivityManager.h"

#include <ESPmDNS.h>
#include <ctype.h>

#include "Logger.h"

namespace
{
    bool isIpv4Literal(const char *host)
    {
        if (host == nullptr || *host == '\0')
        {
            return false;
        }

        for (const char *p = host; *p != '\0'; ++p)
        {
            if (!isdigit(static_cast<unsigned char>(*p)) && *p != '.')
            {
                return false;
            }
        }
        return true;
    }
} // namespace

ConnectivityManager *ConnectivityManager::instance_ = nullptr;

ConnectivityManager::ConnectivityManager(const char *stationSsid,
                                         const char *stationPassword,
                                         const char *apSsid,
                                         const char *apPassword)
    : stationSsid_(stationSsid),
      stationPassword_(stationPassword),
      apSsid_(apSsid),
      apPassword_(apPassword),
      mqttClient_(wifiClient_),
      server_(80)
{
    // 保存实例地址，供静态 MQTT 回调转发使用。
    instance_ = this;
}

void ConnectivityManager::begin()
{
    // 局域网优先：仅启用 STA，通过路由器内网地址访问控制页面。
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    mqttClient_.setCallback(ConnectivityManager::handleMqttDispatch);
    // 缩短套接字超时，避免云端不可达时长时间阻塞主循环。
    mqttClient_.setSocketTimeout(1);

    // 不自动开启 AP；仅尝试连接家庭路由器。
    mode_ = OperatingMode::LocalAP;

    // 非阻塞发起 STA 连接，避免启动阶段长时间不可控。
    connectStation(false);
    // 根据当前连接结果决定是否切到 cloud/local_ap。
    evaluateMode();
    announceAccessUrl();
}

void ConnectivityManager::loop()
{
    // 每轮循环都处理本地网页请求。
    server_.handleClient();

    // 云端消息通道仅在云模式下工作。
    if (mode_ == OperatingMode::Cloud)
    {
        ensureMqttConnected();
        mqttClient_.loop();
    }
    else if (mqttClient_.connected())
    {
        // 非云模式主动断开 MQTT，避免无效占用。
        mqttClient_.disconnect();
    }

    // 定期重评模式，尝试恢复联网链路。
    if (millis() - lastModeCheckMs_ >= 30000)
    {
        lastModeCheckMs_ = millis();
        evaluateMode();
    }
}

void ConnectivityManager::configureCloudMqtt(const char *host,
                                             uint16_t port,
                                             const char *clientId,
                                             const char *user,
                                             const char *password,
                                             const char *controlTopic)
{
    // 保存云端参数；实际连接在循环中延迟重试。
    mqttHost_ = host;
    mqttPort_ = port;
    mqttClientId_ = clientId;
    mqttUser_ = user;
    mqttPassword_ = password;
    mqttControlTopic_ = controlTopic;
    if (mqttHost_ != nullptr)
    {
        mqttClient_.setServer(mqttHost_, mqttPort_);
    }
}

void ConnectivityManager::setMqttHandler(MqttHandler handler)
{
    // 允许上层替换处理器（如切换不同业务逻辑）。
    mqttHandler_ = handler;
}

bool ConnectivityManager::mqttPublish(const char *topic, const String &payload)
{
    // 发布路径不触发连接建立，避免业务发送阻塞网页响应。
    if (!mqttClient_.connected())
    {
        return false;
    }

    // PubSubClient 发送接口要求 C 字符串。
    return mqttClient_.publish(topic, payload.c_str());
}

bool ConnectivityManager::isInternetConnected() const
{
    return WiFi.status() == WL_CONNECTED;
}

bool ConnectivityManager::isCloudMode() const
{
    return mode_ == OperatingMode::Cloud;
}

OperatingMode ConnectivityManager::mode() const
{
    return mode_;
}

IPAddress ConnectivityManager::localIp() const
{
    // 云模式返回路由器分配地址；本地模式返回 AP 地址。
    return mode_ == OperatingMode::Cloud ? WiFi.localIP() : WiFi.softAPIP();
}

String ConnectivityManager::ipString() const
{
    return localIp().toString();
}

String ConnectivityManager::hostName() const
{
    return "esp32-home-server.local";
}

WebServer &ConnectivityManager::webServer()
{
    return server_;
}

void ConnectivityManager::handleMqttDispatch(char *topic, uint8_t *payload, unsigned int length)
{
    // 静态回调转发到实例处理器。
    if (instance_ != nullptr && instance_->mqttHandler_)
    {
        instance_->mqttHandler_(topic, payload, length);
    }
}

void ConnectivityManager::connectStation(bool longWait)
{
    // 未配置联网凭据时直接返回。
    if (stationSsid_ == nullptr || strlen(stationSsid_) == 0)
    {
        return;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }

    // 热点与联网并行模式下重连联网时保持热点在线。
    WiFi.mode(apStarted_ ? WIFI_AP_STA : WIFI_STA);
    const unsigned long now = millis();
    const unsigned long retryIntervalMs = longWait ? 15000UL : 4000UL;
    if (stationConnectInProgress_ && now - lastStationConnectAttemptMs_ < retryIntervalMs)
    {
        return;
    }

    stationConnectInProgress_ = true;
    lastStationConnectAttemptMs_ = now;
    WiFi.begin(stationSsid_, stationPassword_);
}

void ConnectivityManager::evaluateMode()
{
    // 在决定回退模式前先尝试恢复联网。
    if (!isInternetConnected())
    {
        connectStation(false);
    }

    // 联网成功则进入云模式。
    if (isInternetConnected())
    {
        stationConnectInProgress_ = false;
        // STA 联网成功后确保 AP 关闭，避免切到热点模式。
        stopLocalAp();
        mode_ = OperatingMode::Cloud;
        ensureMdnsState();
        announceAccessUrl();
        return;
    }

    // 未连上路由器时不自动开启 AP，等待 STA 重连。
    stopLocalAp();
    mode_ = OperatingMode::LocalAP;
    ensureMdnsState();
    announceAccessUrl();
}

void ConnectivityManager::startLocalAp()
{
    if (apStarted_)
    {
        return;
    }

    // AP+STA 并行：热点可用同时保留 STA 重连能力。
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apSsid_, apPassword_);
    apStarted_ = true;
}

void ConnectivityManager::stopLocalAp()
{
    if (!apStarted_)
    {
        return;
    }

    // 关闭热点后回到 STA 模式。
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    apStarted_ = false;
}

void ConnectivityManager::ensureMdnsState()
{
    if (mode_ == OperatingMode::Cloud && isInternetConnected())
    {
        if (!mdnsStarted_ && MDNS.begin("esp32-home-server"))
        {
            MDNS.addService("http", "tcp", 80);
            mdnsStarted_ = true;
        }
        return;
    }

    if (mdnsStarted_)
    {
        MDNS.end();
        mdnsStarted_ = false;
    }
}

void ConnectivityManager::announceAccessUrl()
{
    const String ip = ipString();
    if (hasAnnouncedAccessUrl_ && mode_ == lastAnnouncedMode_ && ip == lastAnnouncedIp_)
    {
        return;
    }

    hasAnnouncedAccessUrl_ = true;
    lastAnnouncedMode_ = mode_;
    lastAnnouncedIp_ = ip;

    if (mode_ == OperatingMode::Cloud)
    {
        LOG_INFO("NET", "页面访问地址: http://%s/", ip.c_str());
        LOG_INFO("NET", "已联网，可尝试 mDNS 地址: http://%s/", hostName().c_str());
    }
    else if (mode_ == OperatingMode::LocalAP)
    {
        LOG_WARN("NET", "当前未连接路由器，局域网网页暂不可用，正在重连 STA...");
    }
}

bool ConnectivityManager::ensureMqttConnected()
{
    // 仅在云模式且配置完成时才尝试消息连接。
    if (mode_ != OperatingMode::Cloud || !isMqttConfigured())
    {
        return false;
    }

    if (mqttClient_.connected())
    {
        return true;
    }

    const unsigned long now = millis();

    // 限制重连频率，避免紧密重试循环。
    if (now - lastMqttRetryMs_ < 15000UL)
    {
        return false;
    }
    lastMqttRetryMs_ = now;

    // 域名场景使用 DNS 缓存并对失败重试做退避，降低阻塞与日志噪音。
    if (!isIpv4Literal(mqttHost_))
    {
        constexpr unsigned long kDnsRetryBackoffMs = 30000UL;
        constexpr unsigned long kDnsCacheTtlMs = 300000UL;

        const bool cacheExpired = !hasCachedMqttIp_ || (now - lastDnsResolveSuccessMs_ >= kDnsCacheTtlMs);
        if (cacheExpired)
        {
            if (now - lastDnsResolveAttemptMs_ < kDnsRetryBackoffMs)
            {
                return false;
            }
            lastDnsResolveAttemptMs_ = now;

            IPAddress resolved;
            if (WiFi.hostByName(mqttHost_, resolved) != 1)
            {
                LOG_WARN("MQTT", "DNS 解析失败: %s", mqttHost_);
                hasCachedMqttIp_ = false;
                return false;
            }

            cachedMqttIp_ = resolved;
            hasCachedMqttIp_ = true;
            lastDnsResolveSuccessMs_ = now;
        }
        mqttClient_.setServer(cachedMqttIp_, mqttPort_);
    }
    else
    {
        mqttClient_.setServer(mqttHost_, mqttPort_);
    }

    bool connected = false;
    if (mqttUser_ != nullptr && strlen(mqttUser_) > 0)
    {
        // 认证模式连接。
        connected = mqttClient_.connect(mqttClientId_, mqttUser_, mqttPassword_);
    }
    else
    {
        // 匿名连接。
        connected = mqttClient_.connect(mqttClientId_);
    }

    // 连接成功后订阅控制主题。
    if (connected && mqttControlTopic_ != nullptr)
    {
        mqttClient_.subscribe(mqttControlTopic_);
    }

    return connected;
}

bool ConnectivityManager::isMqttConfigured() const
{
    return mqttHost_ != nullptr && strlen(mqttHost_) > 0 && mqttClientId_ != nullptr && strlen(mqttClientId_) > 0;
}
