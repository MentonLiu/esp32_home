#include "ConnectivityManager.h"

ConnectivityManager *ConnectivityManager::instance_ = nullptr;

ConnectivityManager::ConnectivityManager(const char *staSsid, const char *staPassword, const char *apSsid,
                                         const char *apPassword)
    : staSsid_(staSsid),
      staPassword_(staPassword),
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
    tryConnectStation();
    mqttClient_.setCallback(ConnectivityManager::mqttDispatch);
    evaluateMode();
}

void ConnectivityManager::loop()
{
    server_.handleClient();

    if (mode_ == Mode::Cloud)
    {
        mqttClient_.loop();
        ensureMqttConnected();
    }

    if (millis() - lastModeCheckMs_ >= 30000)
    {
        lastModeCheckMs_ = millis();
        evaluateMode();
    }
}

void ConnectivityManager::configureCloudMqtt(const char *host, uint16_t port, const char *clientId, const char *user,
                                             const char *password)
{
    mqttHost_ = host;
    mqttPort_ = port;
    mqttClientId_ = clientId;
    mqttUser_ = user;
    mqttPassword_ = password;
    mqttClient_.setServer(mqttHost_, mqttPort_);
}

void ConnectivityManager::setMqttHandler(MqttHandler handler) { mqttHandler_ = handler; }

bool ConnectivityManager::mqttPublish(const char *topic, const String &payload)
{
    if (!ensureMqttConnected())
    {
        return false;
    }
    return mqttClient_.publish(topic, payload.c_str());
}

bool ConnectivityManager::isInternetConnected() const { return WiFi.status() == WL_CONNECTED; }

bool ConnectivityManager::isCloudMode() const { return mode_ == Mode::Cloud; }

ConnectivityManager::Mode ConnectivityManager::mode() const { return mode_; }

WebServer &ConnectivityManager::webServer() { return server_; }

IPAddress ConnectivityManager::localIP() const
{
    if (mode_ == Mode::Cloud)
    {
        return WiFi.localIP();
    }
    return WiFi.softAPIP();
}

void ConnectivityManager::mqttDispatch(char *topic, uint8_t *payload, unsigned int length)
{
    if (instance_ != nullptr && instance_->mqttHandler_)
    {
        instance_->mqttHandler_(topic, payload, length);
    }
}

void ConnectivityManager::tryConnectStation()
{
    if (staSsid_ == nullptr || strlen(staSsid_) == 0)
    {
        return;
    }

    WiFi.begin(staSsid_, staPassword_);
    const unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000)
    {
        delay(200);
    }
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
    apStarted_ = false;
}

void ConnectivityManager::evaluateMode()
{
    if (!isInternetConnected())
    {
        tryConnectStation();
    }

    if (isInternetConnected())
    {
        stopLocalAp();
        mode_ = Mode::Cloud;
    }
    else
    {
        startLocalAp();
        mode_ = Mode::LocalAP;
    }
}

bool ConnectivityManager::ensureMqttConnected()
{
    if (mode_ != Mode::Cloud || mqttHost_ == nullptr || strlen(mqttHost_) == 0)
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

    if (connected)
    {
        mqttClient_.subscribe("esp32/home/control");
    }

    return connected;
}
