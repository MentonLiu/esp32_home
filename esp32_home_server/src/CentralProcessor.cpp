#include "CentralProcessor.h"

#include <ArduinoJson.h>

#include "pins.h"

namespace
{
    constexpr const char *kStationSsid = "home-WiFi";
    constexpr const char *kStationPassword = "";
    constexpr const char *kApSsid = "esp32-server";
    constexpr const char *kApPassword = "lbl450981";

    HardwareSerial gIrBridgeSerial(2);
} // namespace

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

    sensorDataProcessor_.begin();

    gIrBridgeSerial.begin(ir_.baudRate(), SERIAL_8N1, pins::IR_BRIDGE_UART_RX, pins::IR_BRIDGE_UART_TX);
    commandProcessor_.begin(gIrBridgeSerial);

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

    localProgram_.begin();
    automationEngine_.begin();
}

void CentralProcessor::loop()
{
    net_.loop();

    const bool sampled = sensorDataProcessor_.loop();
    if (sampled && sensorDataProcessor_.shouldPublish())
    {
        net_.mqttPublish(mqtt_upstream::sensorTopic(), sensorDataProcessor_.buildSensorJson());
    }

    automationEngine_.loop(sensorDataProcessor_.latest());

    String irPayload;
    if (commandProcessor_.pollIrBridgeMessage(irPayload))
    {
        publishStatus(mqtt_upstream::statusTopic(), "ir_bridge_rx", irPayload);
    }
}

void CentralProcessor::handleCloudCommand(char *topic, uint8_t *payload, unsigned int length)
{
    String body;
    body.reserve(length);
    for (unsigned int index = 0; index < length; ++index)
    {
        body += static_cast<char>(payload[index]);
    }

    const CommandResult result = commandProcessor_.processCommandJson(body, CommandSource::CloudMqtt);
    publishStatus(mqtt_upstream::statusTopic(),
                  result.accepted ? result.type : "error",
                  String("topic=") + topic + " " + result.message);
}

void CentralProcessor::publishStatus(const char *topic, const String &type, const String &message)
{
    JsonDocument doc;
    doc["type"] = type;
    doc["message"] = message;
    doc["mode"] = modeToString(net_.mode());
    doc["timestamp"] = millis();

    String payload;
    serializeJson(doc, payload);
    net_.mqttPublish(topic, payload);
}
