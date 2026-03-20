#include "CentralProcessor.h"

#include <ArduinoJson.h>

#include "pins.h"

namespace
{
    // 供网络管理器使用的联网与热点凭据。
    constexpr const char *kStationSsid = "home-WiFi";
    constexpr const char *kStationPassword = "";
    constexpr const char *kApSsid = "esp32-server";
    constexpr const char *kApPassword = "lbl450981";

    // 第二路硬件串口专用于红外桥接侧车通信。
    HardwareSerial gIrBridgeSerial(2);
} // 命名空间

CentralProcessor::CentralProcessor()
    : sensorHub_(pins::DHT_DATA, pins::LIGHT_ANALOG, pins::MQ2_ANALOG, pins::FLAME_ANALOG),
      fan_(pins::FAN_PWM),
      curtain_(pins::CURTAIN_SERVO_A, pins::CURTAIN_SERVO_B),
      buzzer_(pins::BUZZER),
      ir_(pins::IR_BRIDGE_UART_RX, pins::IR_BRIDGE_UART_TX, pins::IR_BRIDGE_UART_BAUD),
      net_(kStationSsid, kStationPassword, kApSsid, kApPassword),
      sensorDataProcessor_(sensorHub_),
      commandProcessor_(fan_, curtain_, buzzer_, ir_),
      localProgram_(net_,
                    sensorDataProcessor_,
                    commandProcessor_,
                    [this](const char *topic, const String &type, const String &message)
                    { publishStatus(topic, type, message); }),
      automationEngine_(net_,
                        commandProcessor_,
                        [this](const char *topic, const String &type, const String &message)
                        { publishStatus(topic, type, message); })
{
}

void CentralProcessor::begin()
{
    Serial.begin(115200);

    // 先启动传感器链路，尽早产出可用采样数据。
    sensorDataProcessor_.begin();

    // 初始化红外桥接串口和执行器命令处理器。
    gIrBridgeSerial.begin(ir_.baudRate(), SERIAL_8N1, pins::IR_BRIDGE_UART_RX, pins::IR_BRIDGE_UART_TX);
    commandProcessor_.begin(gIrBridgeSerial);

    // 启动无线联网与热点切换，以及本地网页后端。
    net_.begin();

    const mqtt_upstream::CloudConfig &cloud = mqtt_upstream::cloudConfig();
    net_.configureCloudMqtt(cloud.host,  
                            cloud.port,
                            cloud.clientId,
                            cloud.user,
                            cloud.password,
                            mqtt_upstream::controlTopic());
    net_.setMqttHandler([this](char *topic, uint8_t *payload, unsigned int length)
                        { handleCloudCommand(topic, payload, length); });

    // 注册本地路由并初始化自动化策略。
    localProgram_.begin();
    automationEngine_.begin();
}

void CentralProcessor::loop()
{
    // 维护连接状态并处理网页与消息流量。
    net_.loop();

    // 仅在有新样本且满足节流条件时发布传感器数据。
    const bool sampled = sensorDataProcessor_.loop();
    if (sampled && sensorDataProcessor_.shouldPublish())
    {
        net_.mqttPublish(mqtt_upstream::sensorTopic(), sensorDataProcessor_.buildSensorJson());
    }

    // 使用最新标准化传感数据执行策略引擎。
    automationEngine_.loop(sensorDataProcessor_.latest());

    // 将红外桥接入站消息转发为状态帧。
    String irPayload;
    if (commandProcessor_.pollIrBridgeMessage(irPayload))
    {
        publishStatus(mqtt_upstream::statusTopic(), "ir_bridge_rx", irPayload);
    }
}

void CentralProcessor::handleCloudCommand(char *topic, uint8_t *payload, unsigned int length)
{
    // 将字节载荷转成字符串，便于后续结构化解析。
    String body;
    body.reserve(length);
    for (unsigned int index = 0; index < length; ++index)
    {
        body += static_cast<char>(payload[index]);
    }

    // 处理命令并在状态消息中附带主题上下文。
    const CommandResult result = commandProcessor_.processCommandJson(body, CommandSource::CloudMqtt);
    publishStatus(mqtt_upstream::statusTopic(),
                  result.accepted ? result.type : "error",
                  String("topic=") + topic + " " + result.message);
}

void CentralProcessor::publishStatus(const char *topic, const String &type, const String &message)
{
    // 统一且轻量的状态消息结构。
    JsonDocument doc;
    doc["type"] = type;
    doc["message"] = message;
    doc["mode"] = modeToString(net_.mode());
    doc["timestamp"] = millis();

    String payload;
    serializeJson(doc, payload);
    net_.mqttPublish(topic, payload);
}
