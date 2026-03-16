#ifndef CONNECTIVITY_MANAGER_H
#define CONNECTIVITY_MANAGER_H

#include <Arduino.h>
#include <functional>
#include <PubSubClient.h>
#include <WebServer.h>
#include <WiFi.h>

class ConnectivityManager
{
public:
    enum class Mode : uint8_t
    {
        Cloud = 0,
        LocalAP = 1
    };
    using MqttHandler = std::function<void(char *topic, uint8_t *payload, unsigned int length)>;

    ConnectivityManager(const char *staSsid, const char *staPassword, const char *apSsid, const char *apPassword);

    void begin();
    void loop();

    void configureCloudMqtt(const char *host, uint16_t port, const char *clientId, const char *user = nullptr,
                            const char *password = nullptr);
    void setMqttHandler(MqttHandler handler);

    bool mqttPublish(const char *topic, const String &payload);
    bool isInternetConnected() const;
    bool isCloudMode() const;
    Mode mode() const;

    WebServer &webServer();
    IPAddress localIP() const;

private:
    static void mqttDispatch(char *topic, uint8_t *payload, unsigned int length);
    void tryConnectStation();
    void startLocalAp();
    void stopLocalAp();
    void evaluateMode();
    bool ensureMqttConnected();

    const char *staSsid_;
    const char *staPassword_;
    const char *apSsid_;
    const char *apPassword_;

    const char *mqttHost_ = nullptr;
    const char *mqttClientId_ = nullptr;
    const char *mqttUser_ = nullptr;
    const char *mqttPassword_ = nullptr;
    uint16_t mqttPort_ = 1883;

    WiFiClient wifiClient_;
    PubSubClient mqttClient_;
    WebServer server_;

    bool apStarted_ = false;
    Mode mode_ = Mode::LocalAP;
    unsigned long lastModeCheckMs_ = 0;
    unsigned long lastMqttRetryMs_ = 0;

    MqttHandler mqttHandler_;
    static ConnectivityManager *instance_;
};

#endif