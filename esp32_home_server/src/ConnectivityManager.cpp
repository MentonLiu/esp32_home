// 文件说明：esp32_home_server/src/ConnectivityManager.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "ConnectivityManager.h"

#include <ESPmDNS.h>

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
    instance_ = this;
}

void ConnectivityManager::begin()
{
    // 先以联网模式启动，并开启自动重连。
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    mqttClient_.setCallback(ConnectivityManager::handleMqttDispatch);

    connectStation(true);
    evaluateMode();
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
    mqttHandler_ = handler;
}

bool ConnectivityManager::mqttPublish(const char *topic, const String &payload)
{
    // 发布前先确保连接可用。
    if (!ensureMqttConnected())
    {
        return false;
    }

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
    WiFi.begin(stationSsid_, stationPassword_);

    const unsigned long timeoutMs = longWait ? 15000UL : 4000UL;
    const unsigned long startMs = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startMs < timeoutMs)
    {
        delay(200);
    }
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
        stopLocalAp();
        mode_ = OperatingMode::Cloud;
        ensureMdnsState();
        return;
    }

    // 否则开启本地热点供离线控制。
    startLocalAp();
    mode_ = OperatingMode::LocalAP;
    ensureMdnsState();
}

void ConnectivityManager::startLocalAp()
{
    if (apStarted_)
    {
        return;
    }

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
    if (millis() - lastMqttRetryMs_ < 5000)
    {
        return false;
    }
    lastMqttRetryMs_ = millis();

    bool connected = false;
    if (mqttUser_ != nullptr && strlen(mqttUser_) > 0)
    {
        connected = mqttClient_.connect(mqttClientId_, mqttUser_, mqttPassword_);
    }
    else
    {
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
