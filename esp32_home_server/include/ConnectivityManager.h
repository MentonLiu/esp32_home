/**
 * @file ConnectivityManager.h
 * @brief 网络连接管理头文件
 * 
 * 负责ESP32的网络连接管理，包括：
 * - WiFi Station模式连接
 * - WiFi AP热点模式
 * - MQTT消息队列订阅发布
 * - 简易Web服务器
 * 
 * 工作模式：
 * - Cloud模式: 连接外部WiFi，使用MQTT云端通信
 * - LocalAP模式: WiFi连接失败时启动本地AP，供用户配置
 */

#ifndef CONNECTIVITY_MANAGER_H
#define CONNECTIVITY_MANAGER_H

#include <Arduino.h>
#include <functional>
#include <PubSubClient.h>
#include <WebServer.h>
#include <WiFi.h>

/**
 * @brief 网络连接管理器类
 * @details 统一管理WiFi连接、MQTT通信和Web服务
 */
class ConnectivityManager
{
public:
    /**
     * @brief 运行模式枚举
     */
    enum class Mode : uint8_t
    {
        Cloud = 0,   ///< 云端模式：连接外网，使用MQTT
        LocalAP = 1  ///< 本地AP模式：启动热点供配置
    };

    /**
     * @brief MQTT消息处理函数类型
     * @param topic 消息主题
     * @param payload 消息载荷
     * @param length 载荷长度
     */
    using MqttHandler = std::function<void(char *topic, uint8_t *payload, unsigned int length)>;

    /**
     * @brief 构造函数
     * @param staSsid WiFi Station模式SSID
     * @param staPassword WiFi密码
     * @param apSsid AP模式热点名称
     * @param apPassword AP模式热点密码
     */
    ConnectivityManager(const char *staSsid, const char *staPassword, const char *apSsid, const char *apPassword);

    /**
     * @brief 初始化网络连接
     * 启动WiFi并尝试连接
     */
    void begin();

    /**
     * @brief 主循环处理
     * 处理Web服务器和MQTT连接
     */
    void loop();

    /**
     * @brief 配置MQTT云端连接
     * @param host MQTT服务器地址
     * @param port MQTT服务器端口
     * @param clientId 客户端ID
     * @param user 用户名(可选)
     * @param password 密码(可选)
     */
    void configureCloudMqtt(const char *host, uint16_t port, const char *clientId, const char *user = nullptr,
                            const char *password = nullptr);

    /**
     * @brief 设置MQTT消息处理回调
     * @param handler 处理函数
     */
    void setMqttHandler(MqttHandler handler);

    /**
     * @brief 发布MQTT消息
     * @param topic 主题
     * @param payload 消息内容
     * @return bool 是否发送成功
     */
    bool mqttPublish(const char *topic, const String &payload);

    /**
     * @brief 检查互联网连接状态
     * @return bool 是否已连接互联网
     */
    bool isInternetConnected() const;

    /**
     * @brief 检查是否处于云端模式
     * @return bool 是否为Cloud模式
     */
    bool isCloudMode() const;

    /**
     * @brief 获取当前运行模式
     * @return Mode 当前模式
     */
    Mode mode() const;

    /**
     * @brief 获取Web服务器引用
     * @return WebServer& Web服务器对象
     */
    WebServer &webServer();

    /**
     * @brief 获取本地IP地址
     * @return IPAddress IP地址
     */
    IPAddress localIP() const;

private:
    /**
     * @brief MQTT消息分发回调
     * @details 静态成员函数，用于libmqtt的回调要求
     */
    static void mqttDispatch(char *topic, uint8_t *payload, unsigned int length);

    /**
     * @brief 尝试连接WiFi Station
     */
    void tryConnectStation();

    /**
     * @brief 启动本地AP热点
     */
    void startLocalAp();

    /**
     * @brief 停止本地AP热点
     */
    void stopLocalAp();

    /**
     * @brief 评估并切换运行模式
     * @details 根据互联网连接状态自动切换Cloud/LocalAP模式
     */
    void evaluateMode();

    /**
     * @brief 确保MQTT已连接
     * @return bool 是否已连接
     */
    bool ensureMqttConnected();

    // WiFi配置
    const char *staSsid_;      ///< WiFi SSID
    const char *staPassword_;  ///< WiFi密码
    const char *apSsid_;       ///< AP热点名称
    const char *apPassword_;   ///< AP热点密码

    // MQTT配置
    const char *mqttHost_ = nullptr;     ///< MQTT服务器地址
    const char *mqttClientId_ = nullptr; ///< MQTT客户端ID
    const char *mqttUser_ = nullptr;      ///< MQTT用户名
    const char *mqttPassword_ = nullptr;  ///< MQTT密码
    uint16_t mqttPort_ = 1883;            ///< MQTT端口

    // 网络客户端
    WiFiClient wifiClient_;      ///< WiFi客户端
    PubSubClient mqttClient_;   ///< MQTT客户端
    WebServer server_;          ///< Web服务器(端口80)

    // 状态变量
    bool apStarted_ = false;         ///< AP是否已启动
    Mode mode_ = Mode::LocalAP;      ///< 当前运行模式
    unsigned long lastModeCheckMs_ = 0;    ///< 上次模式检查时间
    unsigned long lastMqttRetryMs_ = 0;     ///< 上次MQTT重连时间

    // 回调函数
    MqttHandler mqttHandler_;              ///< MQTT消息处理函数
    static ConnectivityManager *instance_; ///< 静态实例指针(用于静态回调)
};

#endif
