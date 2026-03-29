// 文件说明：esp32_home_server/src/CentralProcessor.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "CentralProcessor.h"

#include "pins.h"
#include "Logger.h"
#include "MqttUpstream.h"

namespace
{
    // 当前项目固定的联网配置。
    constexpr const char *kStationSsid = "HW-2103";
    constexpr const char *kStationPassword = "20220715";
    constexpr const char *kApSsid = "esp32-server";
    constexpr const char *kApPassword = "lbl450981";
    constexpr const char *kDhtModel = "DHT11";
} // namespace

CentralProcessor::CentralProcessor()
    : sensorHub_(pins::DHT_DATA, pins::LIGHT_ANALOG, pins::MQ2_ANALOG, pins::RAIN_ANALOG),
      fan_(pins::FAN_PWM, 5),
      curtain_(pins::CURTAIN_SERVO_A, pins::CURTAIN_SERVO_B),
      buzzer_(pins::BUZZER),
      net_(kStationSsid, kStationPassword, kApSsid, kApPassword),
      sensorDataProcessor_(sensorHub_),
      commandProcessor_(fan_, curtain_, buzzer_),
      automationEngine_(net_,
                        commandProcessor_,
                        [this](const char *topic, const String &type, const String &message)
                        { publishStatus(topic, type, message); }),
      localProgram_(net_,
                    sensorDataProcessor_,
                    commandProcessor_,
                    [this](const char *topic, const String &type, const String &message)
                    { publishStatus(topic, type, message); })
{
}

void CentralProcessor::begin()
{
    // 串口主要用于日志与调试输出。
    Serial.begin(115200);
    delay(100); // 等待串口初始化完毕

    LOG_INFO("SYSTEM", "=== ESP32 Home Server 启动 ===");
    LOG_INFO("SYSTEM", "编译时间: %s", __TIME__);

    // 1) 先起传感器链路，保证后续状态接口有数据。
    LOG_INFO("INIT", "初始化传感器...");
    sensorDataProcessor_.begin();
    LOG_INFO("INIT", "传感器初始化完成");
    LOG_INFO("INIT", "DHT 配置: model=%s pin=%u read_mode=live_with_cache", kDhtModel, pins::DHT_DATA);

    // 2) 初始化执行器。
    LOG_INFO("INIT", "初始化执行器...");
    commandProcessor_.begin();
    LOG_INFO("INIT", "执行器初始化完成");
    LOG_INFO("INIT", "风扇 PWM: pin=%u channel=%u freq=%luHz", fan_.pin(), fan_.pwmChannel(), fan_.pwmFrequencyHz());
    LOG_INFO("INIT", "执行器通道: 风扇=%u, 舵机=auto, 蜂鸣器 LEDC 通道=6", fan_.pwmChannel());

    // 3) 启动网络与网页服务。
    LOG_INFO("INIT", "初始化网络...");
    net_.begin();
    LOG_INFO("INIT", "网络初始化完成");

    // 配置云端 MQTT（EMQX）。
    const auto &cloud = mqtt_upstream::cloudConfig();
    net_.configureCloudMqtt(cloud.host,
                            cloud.port,
                            cloud.clientId,
                            cloud.user,
                            cloud.password,
                            mqtt_upstream::controlTopic());
    // 下行控制消息统一复用本地 JSON 指令处理器。
    net_.setMqttHandler([this](char *topic, uint8_t *payload, unsigned int length)
                        {
        if (strcmp(topic, mqtt_upstream::controlTopic()) != 0)
        {
            return;
        }

        String requestBody;
        requestBody.reserve(length + 1);
        for (unsigned int i = 0; i < length; ++i)
        {
            requestBody += static_cast<char>(payload[i]);
        }

        const CommandResult result = commandProcessor_.processCommandJson(requestBody, CommandSource::CloudMqtt);
        publishStatus(mqtt_upstream::statusTopic(), result.accepted ? result.type : "error", result.message); });

    // 4) 启动自动化和本地业务程序。
    LOG_INFO("INIT", "启动自动化引擎...");
    automationEngine_.begin();
    LOG_INFO("INIT", "启动本地程序...");
    localProgram_.begin();

    LOG_INFO("SYSTEM", "=== 启动完成，系统就绪 ===");
}

void CentralProcessor::loop()
{
    // 网络层优先推进，保障 Web/MQTT 实时性。
    net_.loop();
    commandProcessor_.loop();

    // 采样并推进发布节流状态。
    sensorDataProcessor_.loop();
    if (sensorDataProcessor_.shouldPublish())
    {
        net_.mqttPublish(mqtt_upstream::sensorTopic(), sensorDataProcessor_.buildSensorJson());
    }

    // 自动化联动依赖最新传感器数据。
    automationEngine_.loop(sensorDataProcessor_.latest());
}

void CentralProcessor::publishStatus(const char *topic, const String &type, const String &message)
{
    // 云模式时把状态同步到 MQTT；本地模式发送失败也不影响串口日志。
    if (topic != nullptr)
    {
        String payload;
        payload.reserve(96);
        payload = "{\"type\":\"" + type + "\",\"message\":\"" + message + "\"}";
        net_.mqttPublish(topic, payload);
    }

    Serial.print("[status] ");
    Serial.print(type);
    Serial.print(" -> ");
    Serial.println(message);
}
