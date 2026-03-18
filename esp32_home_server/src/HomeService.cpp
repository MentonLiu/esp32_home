/**
 * @file HomeService.cpp
 * @brief 家庭智能服务主流程实现
 *
 * 仅保留生命周期和基础初始化逻辑，
 * Web/控制/自动化等细分实现拆分到独立编译单元。
 */

#include "HomeService.h"

#include <ArduinoJson.h>
#include <time.h>
#include <Wire.h>

#include "pins.h"

namespace
{
    // WiFi配置
    const char *kStaSsid = "home-WiFi";    // WiFi名称
    const char *kStaPassword = "";         // WiFi密码(空)
    const char *kApSsid = "esp32-server";  // AP热点名称
    const char *kApPassword = "lbl450981"; // AP热点密码

    // MQTT配置
    const char *kMqttHost = "example.mqtt.server";   // MQTT服务器地址
    const uint16_t kMqttPort = 1883;                 // MQTT端口
    const char *kMqttClientId = "esp32-home-server"; // 客户端ID

    const char *kStatusTopic = "esp32/home/status"; // 状态主题

    // 红外桥接串口：ESP32 Serial2 <-> ESP8266 红外模块
    HardwareSerial gIrBridgeSerial(2);
} // namespace

/**
 * @brief 构造函数
 * 初始化所有子系统的硬件引脚
 */
HomeService::HomeService()
    : sensors_(pins::DHT_DATA, pins::LDR_ANALOG, pins::MQ2_ANALOG, pins::FLAME_ANALOG),
      fan_(pins::RELAY_FAN),
      curtain_(pins::SERVO_1, pins::SERVO_2),
      buzzer_(pins::BUZZER),
      ir_(pins::IR_BRIDGE_UART_RX, pins::IR_BRIDGE_UART_TX, pins::IR_BRIDGE_UART_BAUD),
      net_(kStaSsid, kStaPassword, kApSsid, kApPassword)
{
    // 编译时间作为备用时间基准(无NTP时使用)
    const int hh = (__TIME__[0] - '0') * 10 + (__TIME__[1] - '0');
    const int mm = (__TIME__[3] - '0') * 10 + (__TIME__[4] - '0');
    const int ss = (__TIME__[6] - '0') * 10 + (__TIME__[7] - '0');
    bootBaseSeconds_ = static_cast<uint32_t>(hh * 3600 + mm * 60 + ss);
}

/**
 * @brief 系统初始化
 * 启动所有子系统
 */
void HomeService::begin()
{
    Serial.begin(115200); // 启动串口

    // 初始化RTC（DS3231）I2C总线，用于无网络环境下维持稳定时钟。
    Wire.begin(pins::RTC_I2C_SDA, pins::RTC_I2C_SCL);
    rtcAvailable_ = rtc_.begin();
    if (!rtcAvailable_)
    {
        Serial.println("RTC init failed, fallback to compile-time clock");
    }

    // 初始化ESP32与ESP8266之间的桥接串口。
    // 约定命令采用“单行JSON”格式，ESP8266按行解析并执行红外收发。
    gIrBridgeSerial.begin(ir_.baudRate(), SERIAL_8N1, pins::IR_BRIDGE_UART_RX, pins::IR_BRIDGE_UART_TX);
    ir_.begin(gIrBridgeSerial);

    // 初始化所有传感器和执行器
    sensors_.begin();
    fan_.begin();
    curtain_.begin();
    buzzer_.begin();

    // 初始化网络连接
    net_.begin();
    net_.configureCloudMqtt(kMqttHost, kMqttPort, kMqttClientId);
    // 设置MQTT消息处理回调
    net_.setMqttHandler([this](char *topic, uint8_t *payload, unsigned int length)
                        {
    String body;
    body.reserve(length);
    for (unsigned int i = 0; i < length; ++i) {
        body += static_cast<char>(payload[i]);
    }
    processControlCommand(body);
    publishStatus(kStatusTopic, "command", String("received topic=") + topic); });

    // 配置Web路由
    setupWebRoutes();
    net_.webServer().begin();

    // 如果已连接互联网，配置NTP时间同步
    if (net_.isInternetConnected())
    {
        configTime(8 * 3600, 0, "pool.ntp.org", "ntp.aliyun.com"); // UTC+8
        ntpConfigured_ = true;
    }

    Serial.println("Home service started");
}

/**
 * @brief 主循环
 * 处理网络、传感器、自动化和报警
 */
void HomeService::loop()
{
    net_.loop();              // 处理网络请求
    handleSensorAndPublish(); // 采集并发布传感器数据
    handleAutomation();       // 执行自动化任务
    handleAlerts();           // 检查并处理报警

    // 读取ESP8266上行消息并转发到状态主题，便于调试桥接链路。
    const IRDecodedSignal signal = ir_.receive();
    if (signal.available)
    {
        JsonDocument doc;
        doc["type"] = "ir_bridge_rx";
        doc["message"] = signal.message;
        String payload;
        serializeJson(doc, payload);
        publishStatus(kStatusTopic, "ir", payload);
    }
}
