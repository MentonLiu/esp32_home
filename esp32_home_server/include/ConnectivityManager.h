#ifndef CONNECTIVITY_MANAGER_H
#define CONNECTIVITY_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <WiFi.h>
#include <functional>

#include "SystemContracts.h"

// 负责无线联网与热点失败切换、内置网页服务和消息客户端生命周期。
class ConnectivityManager
{
public:
    using MqttHandler = std::function<void(char *topic, uint8_t *payload, unsigned int length)>;

    ConnectivityManager(const char *stationSsid,
                        const char *stationPassword,
                        const char *apSsid,
                        const char *apPassword);

    void begin();
    void loop();

    void configureCloudMqtt(const char *host,
                            uint16_t port,
                            const char *clientId,
                            const char *user,
                            const char *password,
                            const char *controlTopic);
    void setMqttHandler(MqttHandler handler);

    bool mqttPublish(const char *topic, const String &payload);
    bool isInternetConnected() const;
    bool isCloudMode() const;
    OperatingMode mode() const;
    IPAddress localIp() const;
    String ipString() const;
    // 暴露网页服务器对象，供本地程序注册路由。
    WebServer &webServer();

private:
    // 适配静态回调签名的转发函数。
    static void handleMqttDispatch(char *topic, uint8_t *payload, unsigned int length);

    void connectStation(bool longWait);
    void evaluateMode();
    void startLocalAp();
    void stopLocalAp();
    bool ensureMqttConnected();
    bool isMqttConfigured() const;

    const char *stationSsid_;
    const char *stationPassword_;
    const char *apSsid_;
    const char *apPassword_;

    const char *mqttHost_ = nullptr;
    const char *mqttClientId_ = nullptr;
    const char *mqttUser_ = nullptr;
    const char *mqttPassword_ = nullptr;
    const char *mqttControlTopic_ = nullptr;
    uint16_t mqttPort_ = 1883;

    WiFiClient wifiClient_;
    PubSubClient mqttClient_;
    WebServer server_;
    MqttHandler mqttHandler_;

    OperatingMode mode_ = OperatingMode::LocalAP;
    bool apStarted_ = false;
    unsigned long lastModeCheckMs_ = 0;
    unsigned long lastMqttRetryMs_ = 0;

    // 供静态回调使用的全局单例指针。
    static ConnectivityManager *instance_;
};

#endif
