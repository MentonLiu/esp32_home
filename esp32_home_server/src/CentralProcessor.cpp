// 文件说明：esp32_home_server/src/CentralProcessor.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "CentralProcessor.h"

#include "pins.h"

namespace
{
    constexpr const char *kStationSsid = "iPadmini";
    constexpr const char *kStationPassword = "lbl450981";
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
    Serial.begin(115200);

    sensorDataProcessor_.begin();

    gIrBridgeSerial.begin(ir_.baudRate(), SERIAL_8N1, pins::IR_BRIDGE_UART_RX, pins::IR_BRIDGE_UART_TX);
    commandProcessor_.begin(gIrBridgeSerial);

    net_.begin();

    // MQTT 方案尚未定稿，本轮先不启用实际链路。
    // net_.configureCloudMqtt(...);
    // net_.setMqttHandler(...);

    automationEngine_.begin();
    localProgram_.begin();
}

void CentralProcessor::loop()
{
    net_.loop();

    sensorDataProcessor_.loop();
    sensorDataProcessor_.shouldPublish();

    automationEngine_.loop(sensorDataProcessor_.latest());

    String irPayload;
    if (commandProcessor_.pollIrBridgeMessage(irPayload))
    {
        publishStatus(nullptr, "ir_bridge_rx", irPayload);
    }
}

void CentralProcessor::publishStatus(const char *topic, const String &type, const String &message)
{
    (void)topic;
    Serial.print("[status] ");
    Serial.print(type);
    Serial.print(" -> ");
    Serial.println(message);
}
