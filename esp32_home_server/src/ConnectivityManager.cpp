/**
 * 文件：esp32_home_server/src/ConnectivityManager.cpp
 * 功能说明：
 *   - 实现 WiFi 连接逻辑和失败切换到 AP 模式
 *   - 实现 MQTT 客户端生命周期管理
 *   - 实现 Web 服务器启动和路由注册
 *   - 实现 mDNS 服务发现
 *
 * 核心实现：
 *   - ConnectivityManager::begin() - 初始化所有网络服务
 *   - ConnectivityManager::loop() - 驱动网络状态机
 *   - WiFi 连接失败时自动切换到 AP 模式
 *   - MQTT 自动重连实现（指数退避）
 *   - Web 路由注册和 HTTP 请求分发
 *
 * 依赖：ConnectivityManager.h, Logger.h, WiFi、PubSubClient、WebServer 库
 * 被依赖于：CentralProcessor.cpp, main.cpp
 *
 * 设计细节：
 *   - AP 热点 SSID 前缀："ESP32-Home-"
 *   - MQTT 自动重连间隔最大 60 秒（指数退避）
 *   - WebServer 监听端口 80
 *   - mDNS 主机名："esp32-home"（可通过 esp32-home.local 访问）
 */

#include "ConnectivityManager.h"

#include <ESPmDNS.h>
#include <ctype.h>

#include "Logger.h"

namespace
{
    constexpr unsigned long kModeCheckIntervalCloudMs = 10000UL;
    constexpr unsigned long kModeCheckIntervalRecoveringMs = 3000UL;
    constexpr uint16_t kMqttClientBufferSize = 1024U;
    constexpr unsigned long kPublishWarnThrottleMs = 5000UL;

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
    // 局域网优先：通过家庭主路由器访问本地网页。
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    mqttClient_.setCallback(ConnectivityManager::handleMqttDispatch);
    if (!mqttClient_.setBufferSize(kMqttClientBufferSize))
    {
        LOG_WARN("MQTT", "缓冲区设置失败，当前 size=%u", mqttClient_.getBufferSize());
    }
    else
    {
        LOG_INFO("MQTT", "缓冲区已设置为 %u", mqttClient_.getBufferSize());
    }

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
    // 启动和离线恢复阶段用更短周期，减少进入云模式和 MQTT 可用的等待时间。
    const unsigned long modeCheckIntervalMs =
        (mode_ == OperatingMode::Cloud && isInternetConnected()) ? kModeCheckIntervalCloudMs : kModeCheckIntervalRecoveringMs;
    if (millis() - lastModeCheckMs_ >= modeCheckIntervalMs)
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
    static unsigned long lastWarnMs = 0;
    const unsigned long now = millis();

    // 本地网页优先：发布路径不主动触发云端重连，避免阻塞 Web 响应。
    if (!mqttClient_.connected())
    {
        if (now - lastWarnMs >= kPublishWarnThrottleMs)
        {
            LOG_WARN("MQTT", "发布失败(未连接): topic=%s len=%u state=%d cloudMode=%d",
                     topic != nullptr ? topic : "<null>",
                     static_cast<unsigned int>(payload.length()),
                     mqttClient_.state(),
                     isCloudMode() ? 1 : 0);
            lastWarnMs = now;
        }
        return false;
    }

    // PubSubClient 发送接口要求 C 字符串。
    const bool ok = mqttClient_.publish(topic, payload.c_str());
    if (!ok && now - lastWarnMs >= kPublishWarnThrottleMs)
    {
        LOG_WARN("MQTT", "发布失败: topic=%s len=%u state=%d buffer=%u",
                 topic != nullptr ? topic : "<null>",
                 static_cast<unsigned int>(payload.length()),
                 mqttClient_.state(),
                 mqttClient_.getBufferSize());
        lastWarnMs = now;
    }
    return ok;
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
        stopLocalAp();
        mode_ = OperatingMode::Cloud;
        ensureMdnsState();
        announceAccessUrl();
        return;
    }

    // 联网失败，启动本地 AP 作为兜底。
    startLocalAp();
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

    // 限制重连频率，避免紧密重试循环。
    if (millis() - lastMqttRetryMs_ < mqttRetryBackoffMs_)
    {
        return false;
    }
    lastMqttRetryMs_ = millis();

    // 域名场景先做一次显式 DNS 解析，失败则本轮放弃 MQTT，不影响本地网页可用性。
    if (!isIpv4Literal(mqttHost_))
    {
        IPAddress resolved;
        if (WiFi.hostByName(mqttHost_, resolved) != 1)
        {
            LOG_WARN("MQTT", "DNS 解析失败: %s", mqttHost_);
            mqttRetryBackoffMs_ = 30000;
            return false;
        }
        mqttClient_.setServer(resolved, mqttPort_);
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
        mqttRetryBackoffMs_ = 5000;
    }
    else if (!connected)
    {
        mqttRetryBackoffMs_ = 30000;
        LOG_WARN("MQTT", "连接失败(state=%d host=%s port=%u)，30 秒后重试",
                 mqttClient_.state(),
                 mqttHost_ != nullptr ? mqttHost_ : "<null>",
                 mqttPort_);
    }

    return connected;
}

bool ConnectivityManager::isMqttConfigured() const
{
    return mqttHost_ != nullptr && strlen(mqttHost_) > 0 && mqttClientId_ != nullptr && strlen(mqttClientId_) > 0;
}
