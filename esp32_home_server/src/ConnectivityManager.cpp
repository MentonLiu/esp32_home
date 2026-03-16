/**
 * @file ConnectivityManager.cpp
 * @brief 网络连接管理实现文件
 * 
 * 实现WiFi连接、MQTT通信、Web服务器等功能的具体逻辑
 */

#include "ConnectivityManager.h"

// 静态实例指针初始化，用于MQTT回调
ConnectivityManager *ConnectivityManager::instance_ = nullptr;

/**
 * @brief 构造函数
 * 初始化WiFi和MQTT客户端配置
 */
ConnectivityManager::ConnectivityManager(const char *staSsid, const char *staPassword, const char *apSsid,
                                         const char *apPassword)
    : staSsid_(staSsid),
      staPassword_(staPassword),
      apSsid_(apSsid),
      apPassword_(apPassword),
      mqttClient_(wifiClient_),  // 将WiFiClient传递给MQTT客户端
      server_(80)                // Web服务器监听80端口
{
    instance_ = this;  // 保存实例指针供静态回调使用
}

/**
 * @brief 初始化网络连接
 * 设置WiFi模式并尝试连接
 */
void ConnectivityManager::begin()
{
    WiFi.mode(WIFI_STA);              // 设置为Station模式
    tryConnectStation();              // 尝试连接WiFi
    mqttClient_.setCallback(ConnectivityManager::mqttDispatch);  // 设置MQTT回调
    evaluateMode();                   // 评估并确定运行模式
}

/**
 * @brief 主循环处理
 * 处理Web请求和MQTT连接维护
 */
void ConnectivityManager::loop()
{
    server_.handleClient();  // 处理Web服务器请求

    // 仅在云端模式下维护MQTT连接
    if (mode_ == Mode::Cloud)
    {
        mqttClient_.loop();         // 处理MQTT消息
        ensureMqttConnected();       // 确保MQTT连接正常
    }

    // 每30秒评估一次运行模式
    if (millis() - lastModeCheckMs_ >= 30000)
    {
        lastModeCheckMs_ = millis();
        evaluateMode();
    }
}

/**
 * @brief 配置MQTT云端连接参数
 */
void ConnectivityManager::configureCloudMqtt(const char *host, uint16_t port, const char *clientId, const char *user,
                                             const char *password)
{
    mqttHost_ = host;
    mqttPort_ = port;
    mqttClientId_ = clientId;
    mqttUser_ = user;
    mqttPassword_ = password;
    mqttClient_.setServer(mqttHost_, mqttPort_);  // 设置MQTT服务器
}

void ConnectivityManager::setMqttHandler(MqttHandler handler) { mqttHandler_ = handler; }

/**
 * @brief 发布MQTT消息
 * @return bool 发送是否成功
 */
bool ConnectivityManager::mqttPublish(const char *topic, const String &payload)
{
    if (!ensureMqttConnected())  // 确保MQTT已连接
    {
        return false;
    }
    return mqttClient_.publish(topic, payload.c_str());
}

bool ConnectivityManager::isInternetConnected() const { return WiFi.status() == WL_CONNECTED; }

bool ConnectivityManager::isCloudMode() const { return mode_ == Mode::Cloud; }

ConnectivityManager::Mode ConnectivityManager::mode() const { return mode_; }

WebServer &ConnectivityManager::webServer() { return server_; }

/**
 * @brief 获取本地IP地址
 * @return IPAddress 根据当前模式返回对应IP地址
 */
IPAddress ConnectivityManager::localIP() const
{
    if (mode_ == Mode::Cloud)
    {
        return WiFi.localIP();   // 返回Station IP
    }
    return WiFi.softAPIP();       // 返回AP IP
}

/**
 * @brief MQTT消息分发回调
 * @details 静态函数，将消息转发给实例的处理器
 */
void ConnectivityManager::mqttDispatch(char *topic, uint8_t *payload, unsigned int length)
{
    if (instance_ != nullptr && instance_->mqttHandler_)
    {
        instance_->mqttHandler_(topic, payload, length);
    }
}

/**
 * @brief 尝试连接WiFi Station
 * @details 最长等待15秒尝试连接
 */
void ConnectivityManager::tryConnectStation()
{
    if (staSsid_ == nullptr || strlen(staSsid_) == 0)  // 无SSID配置则跳过
    {
        return;
    }

    WiFi.begin(staSsid_, staPassword_);  // 开始连接
    const unsigned long start = millis();
    // 等待连接，最多15秒
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000)
    {
        delay(200);  // 每200ms检查一次
    }
}

/**
 * @brief 启动本地AP热点
 * @details 仅在未启动时启动，提供配置入口
 */
void ConnectivityManager::startLocalAp()
{
    if (apStarted_)  // 已启动则跳过
    {
        return;
    }
    WiFi.mode(WIFI_AP_STA);          // 设为AP+Station模式
    WiFi.softAP(apSsid_, apPassword_);  // 启动热点
    apStarted_ = true;
}

/**
 * @brief 停止本地AP热点
 */
void ConnectivityManager::stopLocalAp()
{
    if (!apStarted_)  // 未启动则跳过
    {
        return;
    }
    WiFi.softAPdisconnect(true);  // 断开AP连接
    apStarted_ = false;
}

/**
 * @brief 评估并确定运行模式
 * @details 根据WiFi连接状态自动切换：
 * - 能连接WiFi → Cloud模式，停止AP
 * - 无法连接WiFi → LocalAP模式，启动AP
 */
void ConnectivityManager::evaluateMode()
{
    // 尝试重新连接WiFi(可能临时断开)
    if (!isInternetConnected())
    {
        tryConnectStation();
    }

    // 根据连接状态决定模式
    if (isInternetConnected())
    {
        stopLocalAp();  // 有网则关闭AP
        mode_ = Mode::Cloud;
    }
    else
    {
        startLocalAp();  // 无网则启动AP
        mode_ = Mode::LocalAP;
    }
}

/**
 * @brief 确保MQTT处于连接状态
 * @details 处理连接、重连和订阅
 */
bool ConnectivityManager::ensureMqttConnected()
{
    // 检查是否需要MQTT
    if (mode_ != Mode::Cloud || mqttHost_ == nullptr || strlen(mqttHost_) == 0)
    {
        return false;
    }

    // 已连接则直接返回
    if (mqttClient_.connected())
    {
        return true;
    }

    // 5秒内不重复尝试连接(防止频繁重连)
    if (millis() - lastMqttRetryMs_ < 5000)
    {
        return false;
    }
    lastMqttRetryMs_ = millis();

    // 尝试连接MQTT服务器
    bool connected = false;
    if (mqttUser_ != nullptr && strlen(mqttUser_) > 0)
    {
        // 带用户名密码的连接
        connected = mqttClient_.connect(mqttClientId_, mqttUser_, mqttPassword_);
    }
    else
    {
        // 仅用客户端ID连接
        connected = mqttClient_.connect(mqttClientId_);
    }

    // 连接成功则订阅控制主题
    if (connected)
    {
        mqttClient_.subscribe("esp32/home/control");
    }

    return connected;
}
