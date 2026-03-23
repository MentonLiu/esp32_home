// 文件说明：esp32_home_server/include/ConnectivityManager.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

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
    // MQTT 消息回调类型。
    using MqttHandler = std::function<void(char *topic, uint8_t *payload, unsigned int length)>;

    ConnectivityManager(const char *stationSsid,
                        const char *stationPassword,
                        const char *apSsid,
                        const char *apPassword);

    // 初始化 Wi-Fi/MQTT/WebServer。
    void begin();
    // 主循环：处理 Web 请求、MQTT 心跳与模式切换。
    void loop();

    // 配置云端 MQTT 参数（不会立即阻塞连接）。
    void configureCloudMqtt(const char *host,
                            uint16_t port,
                            const char *clientId,
                            const char *user,
                            const char *password,
                            const char *controlTopic);
    // 注册 MQTT 控制消息处理器。
    void setMqttHandler(MqttHandler handler);

    // 对外发布 MQTT 消息。
    bool mqttPublish(const char *topic, const String &payload);
    // 判断设备当前是否联网。
    bool isInternetConnected() const;
    // 是否处于云模式。
    bool isCloudMode() const;
    // 当前运行模式。
    OperatingMode mode() const;
    // 当前生效 IP（云模式为 STA IP，本地模式为 AP IP）。
    IPAddress localIp() const;
    // 字符串格式 IP。
    String ipString() const;
    // mDNS 主机名。
    String hostName() const;
    // 暴露网页服务器对象，供本地程序注册路由。
    WebServer &webServer();

private:
    // 适配静态回调签名的转发函数。
    static void handleMqttDispatch(char *topic, uint8_t *payload, unsigned int length);

    // 尝试连接家庭路由器。
    void connectStation(bool longWait);
    // 评估当前模式（cloud/local_ap）并切换。
    void evaluateMode();
    // 启动本地热点。
    void startLocalAp();
    // 关闭本地热点。
    void stopLocalAp();
    // 根据模式启停 mDNS。
    void ensureMdnsState();
    // 保证 MQTT 已连接（含节流重试）。
    bool ensureMqttConnected();
    // MQTT 参数是否配置完整。
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
    bool mdnsStarted_ = false;
    unsigned long lastModeCheckMs_ = 0;
    unsigned long lastMqttRetryMs_ = 0;

    // 供静态回调使用的全局单例指针。
    static ConnectivityManager *instance_;
};

#endif
