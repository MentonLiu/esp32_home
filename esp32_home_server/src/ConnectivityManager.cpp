#include "ConnectivityManager.h"

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
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    mqttClient_.setCallback(ConnectivityManager::handleMqttDispatch);

    connectStation(true);
    evaluateMode();
}

void ConnectivityManager::loop()
{
    server_.handleClient();

    if (mode_ == OperatingMode::Cloud)
    {
        ensureMqttConnected();
        mqttClient_.loop();
    }
    else if (mqttClient_.connected())
    {
        mqttClient_.disconnect();
    }

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

WebServer &ConnectivityManager::webServer()
{
    return server_;
}

void ConnectivityManager::handleMqttDispatch(char *topic, uint8_t *payload, unsigned int length)
{
    if (instance_ != nullptr && instance_->mqttHandler_)
    {
        instance_->mqttHandler_(topic, payload, length);
    }
}

void ConnectivityManager::connectStation(bool longWait)
{
    if (stationSsid_ == nullptr || strlen(stationSsid_) == 0)
    {
        return;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }

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
    if (!isInternetConnected())
    {
        connectStation(false);
    }

    if (isInternetConnected())
    {
        stopLocalAp();
        mode_ = OperatingMode::Cloud;
        return;
    }

    startLocalAp();
    mode_ = OperatingMode::LocalAP;
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

bool ConnectivityManager::ensureMqttConnected()
{
    if (mode_ != OperatingMode::Cloud || !isMqttConfigured())
    {
        return false;
    }

    if (mqttClient_.connected())
    {
        return true;
    }

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
