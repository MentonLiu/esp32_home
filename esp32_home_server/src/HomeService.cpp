/**
 * @file HomeService.cpp
 * @brief 家庭智能服务实现文件
 *
 * 实现完整的智能家居服务逻辑：
 * - 系统初始化与配置
 * - 传感器数据采集与发布
 * - 自动化控制(定时窗帘)
 * - 异常检测与报警
 * - Web界面与MQTT控制
 */

#include "HomeService.h"

#include <ArduinoJson.h>
#include <DHT.h>
#include <time.h>

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

    // MQTT主题
    const char *kSensorTopic = "esp32/home/sensors"; // 传感器数据主题
    const char *kStatusTopic = "esp32/home/status";  // 状态主题
    const char *kAlarmTopic = "esp32/home/alarm";    // 报警主题

    // 红外桥接串口：ESP32 Serial2 <-> ESP8266 红外模块
    HardwareSerial gIrBridgeSerial(2);
} // namespace

/**
 * @brief 构造函数
 * 初始化所有子系统的硬件引脚
 */
HomeService::HomeService()
    : sensors_(pins::DHT_DATA, DHT22, pins::LDR_ANALOG, pins::MQ2_ANALOG, pins::FLAME_ANALOG),
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
        DynamicJsonDocument doc(256);
        doc["type"] = "ir_bridge_rx";
        doc["message"] = signal.message;
        String payload;
        serializeJson(doc, payload);
        publishStatus(kStatusTopic, "ir", payload);
    }
}

/**
 * @brief 配置Web服务器路由
 * 提供Web控制界面和API
 */
void HomeService::setupWebRoutes()
{
    // 首页 - Web控制界面
    net_.webServer().on("/", HTTP_GET, [this]()
                        {
    const char html[] = R"HTML(
<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>ESP32 Home Console</title>
  <style>
    :root { --bg:#f4f7f1; --panel:#ffffff; --text:#1f2a1f; --accent:#2f7d32; --warn:#cc7a00; --danger:#c62828; }
    body { margin:0; font-family:"Trebuchet MS", "Segoe UI", sans-serif; background:linear-gradient(160deg,#e6efe0,#fdf8ec); color:var(--text); }
    .wrap { max-width:980px; margin:0 auto; padding:20px; }
    .card { background:var(--panel); border-radius:16px; padding:16px; box-shadow:0 8px 24px rgba(0,0,0,0.08); margin-bottom:14px; }
    h1 { margin:0 0 12px; font-size:1.6rem; }
    .grid { display:grid; grid-template-columns:repeat(auto-fit,minmax(180px,1fr)); gap:10px; }
    .k { font-size:0.86rem; opacity:0.7; }
    .v { font-size:1.2rem; font-weight:700; }
    button { border:0; border-radius:10px; padding:10px 14px; margin:4px; background:var(--accent); color:#fff; font-weight:700; }
    .warn { background:var(--warn); }
    .danger { background:var(--danger); }
    input[type=range] { width:100%; }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card"><h1>ESP32 Home Console</h1><div id="net"></div></div>
    <div class="card grid" id="sensorGrid"></div>
    <div class="card">
      <div><b>Curtain</b></div>
      <input id="curtainRange" type="range" min="0" max="180" value="0" />
      <div>
        <button onclick="sendCmd({device:'curtain',preset:0})">Close</button>
        <button onclick="sendCmd({device:'curtain',preset:2})">Half</button>
        <button onclick="sendCmd({device:'curtain',preset:4})">Open</button>
      </div>
    </div>
    <div class="card">
      <div><b>Fan</b></div>
      <div>
        <button onclick="sendCmd({device:'fan',mode:'off'})" class="danger">Off</button>
        <button onclick="sendCmd({device:'fan',mode:'low'})">Low</button>
        <button onclick="sendCmd({device:'fan',mode:'medium'})">Medium</button>
        <button onclick="sendCmd({device:'fan',mode:'high'})" class="warn">High</button>
      </div>
    </div>
  </div>
<script>
async function fetchStatus() {
  const res = await fetch('/api/status');
  const data = await res.json();
  document.getElementById('net').textContent = `mode=${data.mode} ip=${data.ip}`;
  document.getElementById('sensorGrid').innerHTML = `
    <div><div class='k'>temperature</div><div class='v'>${data.temperatureC} C</div></div>
    <div><div class='k'>humidity</div><div class='v'>${data.humidityPercent} %</div></div>
    <div><div class='k'>light</div><div class='v'>${data.lightPercent} %</div></div>
    <div><div class='k'>smoke</div><div class='v'>${data.smokeLevel} (${data.mq2Percent}%)</div></div>
    <div><div class='k'>flame</div><div class='v'>${data.flameDetected ? 'YES':'NO'}</div></div>
    <div><div class='k'>fan</div><div class='v'>${data.fanSpeedPercent}%</div></div>
    <div><div class='k'>curtain</div><div class='v'>${data.curtainAngle} deg</div></div>
  `;
  document.getElementById('curtainRange').value = data.curtainAngle;
}

async function sendCmd(body) {
  await fetch('/api/control', { method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(body) });
}

document.getElementById('curtainRange').addEventListener('input', (e) => {
  sendCmd({device:'curtain', angle:Number(e.target.value)});
});

setInterval(fetchStatus, 500);
fetchStatus();
</script>
</body>
</html>
)HTML";
    net_.webServer().send(200, "text/html", html); });

    // API: 获取状态
    net_.webServer().on("/api/status", HTTP_GET, [this]()
                        { net_.webServer().send(200, "application/json", buildStatusJson(true)); });

    // API: 控制命令
    net_.webServer().on("/api/control", HTTP_POST, [this]()
                        {
    processControlCommand(net_.webServer().arg("plain"));
    net_.webServer().send(200, "application/json", "{\"ok\":true}"); });
}

/**
 * @brief 处理控制命令
 * 解析JSON命令并执行相应操作
 * 支持设备: fan, curtain, ir
 */
void HomeService::processControlCommand(const String &jsonText)
{
    DynamicJsonDocument doc(512);
    const DeserializationError err = deserializeJson(doc, jsonText);
    if (err)
    {
        publishStatus(kStatusTopic, "error", String("invalid_control_json:") + err.c_str());
        return;
    }

    const String device = doc["device"] | "";

    // 风扇控制
    if (device == "fan")
    {
        const String mode = doc["mode"] | "";
        if (mode == "off")
        {
            fan_.setMode(FanMode::Off);
        }
        else if (mode == "low")
        {
            fan_.setMode(FanMode::Low);
        }
        else if (mode == "medium")
        {
            fan_.setMode(FanMode::Medium);
        }
        else if (mode == "high")
        {
            fan_.setMode(FanMode::High);
        }

        // 支持自定义转速
        if (doc.containsKey("speedPercent"))
        {
            fan_.setSpeedPercent(doc["speedPercent"].as<uint8_t>());
        }
        return;
    }

    // 窗帘控制
    if (device == "curtain")
    {
        // 自定义角度
        if (doc.containsKey("angle"))
        {
            curtain_.setAngle(doc["angle"].as<uint8_t>());
        }
        // 预设等级
        if (doc.containsKey("preset"))
        {
            curtain_.setPresetLevel(doc["preset"].as<uint8_t>());
        }
        return;
    }

    // 红外控制
    if (device == "ir")
    {
        const String action = doc["action"] | "";

        // 通用协议命令，便于后续新增协议
        if (action == "send")
        {
            const String protocol = doc["protocol"] | "NEC";
            const uint32_t address = doc["address"] | 0;
            const uint32_t command = doc["command"] | 0;
            const uint8_t repeats = doc["repeats"] | 0;
            const bool ok = ir_.sendProtocolCommand(protocol, address, command, repeats);
            publishStatus(kStatusTopic, ok ? "ir_command" : "error", ok ? "ir_protocol_sent" : "ir_protocol_send_failed");
            return;
        }

        // 直接下发完整JSON命令，提供最高扩展性
        if (action == "send_json")
        {
            const String commandJson = doc["commandJson"] | "";
            const bool ok = commandJson.length() > 0 && ir_.sendJsonCommand(commandJson);
            publishStatus(kStatusTopic, ok ? "ir_command" : "error", ok ? "ir_json_sent" : "ir_json_send_failed");
            return;
        }

        // 扩展动作命令，args由ESP8266端定义语义
        if (action.length() > 0)
        {
            String args = "{}";
            if (doc.containsKey("args"))
            {
                serializeJson(doc["args"], args);
            }
            const bool ok = ir_.sendActionCommand(action, args);
            publishStatus(kStatusTopic, ok ? "ir_command" : "error", ok ? "ir_action_sent" : "ir_action_send_failed");
            return;
        }

        publishStatus(kStatusTopic, "error", "ir_action_missing");
        return;
    }

    publishStatus(kStatusTopic, "error", "unknown_device");
}

/**
 * @brief 处理传感器数据采集与发布
 * 定期读取传感器并通过MQTT发布
 */
void HomeService::handleSensorAndPublish()
{
    // 更新传感器数据
    if (!sensors_.update())
    {
        return;
    }

    // 发布间隔控制(500ms)
    if (millis() - lastSensorPublishMs_ < 500)
    {
        return;
    }
    lastSensorPublishMs_ = millis();

    // 构建传感器数据JSON
    const SensorSnapshot &s = sensors_.snapshot();
    DynamicJsonDocument doc(512);
    doc["sensorType"] = "home_snapshot";
    doc["timestamp"] = s.timestamp;
    doc["temperatureC"] = s.temperatureC;
    doc["humidityPercent"] = s.humidityPercent;
    doc["lightPercent"] = s.lightPercent;
    doc["mq2Percent"] = s.mq2Percent;
    doc["smokeLevel"] = s.smokeLevel;
    doc["flameDetected"] = s.flameDetected;
    doc["error"] = s.hasError;
    if (s.hasError)
    {
        doc["errorMessage"] = s.errorMessage;
    }

    String payload;
    serializeJson(doc, payload);
    net_.mqttPublish(kSensorTopic, payload);
}

/**
 * @brief 处理自动化任务
 * 定时自动开关窗帘
 */
void HomeService::handleAutomation()
{
    // 执行间隔控制(1秒)
    if (millis() - lastAutomationMs_ < 1000)
    {
        return;
    }
    lastAutomationMs_ = millis();

    // 尝试配置NTP(如果之前未成功)
    if (net_.isInternetConnected() && !ntpConfigured_)
    {
        configTime(8 * 3600, 0, "pool.ntp.org", "ntp.aliyun.com");
        ntpConfigured_ = true;
    }

    const int hour = currentHour();
    const int minute = currentMinute();
    const uint32_t dayIndex = currentDayIndex();

    // 早上7点自动开窗帘
    if (hour == 7 && minute == 0 && lastOpenDay_ != dayIndex)
    {
        curtain_.setPresetLevel(4); // 全开
        lastOpenDay_ = dayIndex;
        publishStatus(kStatusTopic, "automation", "curtain_open_07_00");
    }

    // 晚上10点自动关窗帘
    if (hour == 22 && minute == 0 && lastCloseDay_ != dayIndex)
    {
        curtain_.setPresetLevel(0); // 全关
        lastCloseDay_ = dayIndex;
        publishStatus(kStatusTopic, "automation", "curtain_close_22_00");
    }
}

/**
 * @brief 处理报警逻辑
 * 检测异常并触发相应动作
 */
void HomeService::handleAlerts()
{
    const SensorSnapshot &s = sensors_.snapshot();

    // 烟雾浓度高时自动开启风扇
    if (s.mq2Percent >= 75)
    {
        fan_.setMode(FanMode::High);
    }

    // 烟雾浓度极高时发出蜂鸣警报(每秒一次)
    if (s.mq2Percent >= 90 && millis() - lastHighSmokeBeepMs_ >= 1000)
    {
        lastHighSmokeBeepMs_ = millis();
        buzzer_.beep(2600, 80);
    }

    // 火焰检测处理
    if (s.flameDetected)
    {
        // 首次检测到火焰，记录时间
        if (flameDetectedSinceMs_ == 0)
        {
            flameDetectedSinceMs_ = millis();
            fireAlarmReported_ = false;
        }

        // 火焰持续45秒后开始警报模式
        const unsigned long flameDuration = millis() - flameDetectedSinceMs_;
        if (flameDuration >= 45000 && millis() - lastFlamePatternMs_ >= 2500)
        {
            lastFlamePatternMs_ = millis();
            buzzer_.patternShortShortLong(); // 播放警报模式
        }

        // 火焰持续5分钟后上报火警服务
        if (flameDuration >= 300000 && !fireAlarmReported_ && net_.isCloudMode())
        {
            fireAlarmReported_ = true;
            publishStatus(kAlarmTopic, "fire", "flame_over_5min_fire_service_alert");
        }
    }
    else
    {
        // 火焰消失，重置状态
        flameDetectedSinceMs_ = 0;
        fireAlarmReported_ = false;
    }
}

/**
 * @brief 发布状态消息到MQTT
 */
void HomeService::publishStatus(const char *topic, const String &type, const String &message) const
{
    DynamicJsonDocument doc(512);
    doc["type"] = type;
    doc["message"] = message;
    doc["timestamp"] = millis();

    String payload;
    serializeJson(doc, payload);
    const_cast<ConnectivityManager &>(net_).mqttPublish(topic, payload);
}

/**
 * @brief 构建状态JSON
 * 用于Web API响应
 */
String HomeService::buildStatusJson(bool includeMeta) const
{
    const SensorSnapshot &s = sensors_.snapshot();

    DynamicJsonDocument doc(768);
    // 包含网络信息
    if (includeMeta)
    {
        doc["mode"] = net_.isCloudMode() ? "cloud" : "local_ap";
        doc["ip"] = net_.localIP().toString();
    }

    // 传感器数据
    doc["temperatureC"] = s.temperatureC;
    doc["humidityPercent"] = s.humidityPercent;
    doc["lightPercent"] = s.lightPercent;
    doc["mq2Percent"] = s.mq2Percent;
    doc["smokeLevel"] = s.smokeLevel;
    doc["flameDetected"] = s.flameDetected;
    // 执行器状态
    doc["fanSpeedPercent"] = fan_.speedPercent();
    doc["curtainAngle"] = curtain_.angle();
    doc["error"] = s.hasError;
    doc["errorMessage"] = s.errorMessage;

    String out;
    serializeJson(doc, out);
    return out;
}

/**
 * @brief 获取当前小时
 * 优先使用NTP时间，其次使用编译时间估算
 */
int HomeService::currentHour() const
{
    if (ntpConfigured_)
    {
        time_t now = time(nullptr);
        if (now > 100000) // NTP时间有效
        {
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            return timeinfo.tm_hour;
        }
    }

    // 无NTP时使用启动后的运行时间估算
    const uint32_t seconds = bootBaseSeconds_ + millis() / 1000;
    return (seconds / 3600) % 24;
}

/**
 * @brief 获取当前分钟
 */
int HomeService::currentMinute() const
{
    if (ntpConfigured_)
    {
        time_t now = time(nullptr);
        if (now > 100000)
        {
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            return timeinfo.tm_min;
        }
    }

    const uint32_t seconds = bootBaseSeconds_ + millis() / 1000;
    return (seconds / 60) % 60;
}

/**
 * @brief 获取当前天索引
 * 用于判断日期变化
 */
uint32_t HomeService::currentDayIndex() const
{
    if (ntpConfigured_)
    {
        time_t now = time(nullptr);
        if (now > 100000)
        {
            return static_cast<uint32_t>(now / 86400);
        }
    }

    const uint32_t seconds = bootBaseSeconds_ + millis() / 1000;
    return seconds / 86400;
}
