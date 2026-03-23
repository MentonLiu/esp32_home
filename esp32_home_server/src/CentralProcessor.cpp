// 文件说明：esp32_home_server/src/CentralProcessor.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "CentralProcessor.h"

#include "pins.h"

namespace
{
    // 当前项目固定的联网配置。
    constexpr const char *kStationSsid = "iPadmini";
    constexpr const char *kStationPassword = "lbl450981";
    constexpr const char *kApSsid = "esp32-server";
    constexpr const char *kApPassword = "lbl450981";

    // 使用 UART2 与 ESP8266 红外桥接板通信。
    HardwareSerial gIrBridgeSerial(2);
} // namespace

CentralProcessor::CentralProcessor()
    : sensorHub_(pins::DHT_DATA, pins::LIGHT_ANALOG, pins::MQ2_ANALOG, pins::FLAME_ANALOG),
      fan_(pins::FAN_PWM, 4),
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
    // 串口主要用于日志与调试输出。
    Serial.begin(115200);

    // 1) 先起传感器链路，保证后续状态接口有数据。
    sensorDataProcessor_.begin();

    // 2) 初始化红外桥接串口与所有执行器。
    gIrBridgeSerial.begin(ir_.baudRate(), SERIAL_8N1, pins::IR_BRIDGE_UART_RX, pins::IR_BRIDGE_UART_TX);
    commandProcessor_.begin(gIrBridgeSerial);

    // 3) 启动网络与网页服务。
    net_.begin();

    // MQTT 方案尚未定稿，本轮先不启用实际链路。
    // net_.configureCloudMqtt(...);
    // net_.setMqttHandler(...);

    // 4) 启动自动化和本地业务程序。
    automationEngine_.begin();
    localProgram_.begin();
}

void CentralProcessor::loop()
{
    // 网络层优先推进，保障 Web/MQTT 实时性。
    net_.loop();

    // 采样并推进发布节流状态。
    sensorDataProcessor_.loop();
    sensorDataProcessor_.shouldPublish();

    // 自动化联动依赖最新传感器数据。
    automationEngine_.loop(sensorDataProcessor_.latest());

    // 轮询红外桥接上行消息。
    String irPayload;
    if (commandProcessor_.pollIrBridgeMessage(irPayload))
    {
        publishStatus(nullptr, "ir_bridge_rx", irPayload);
    }
}

void CentralProcessor::publishStatus(const char *topic, const String &type, const String &message)
{
    // 当前阶段仅串口打印，topic 预留给未来 MQTT 上报。
    (void)topic;
    Serial.print("[status] ");
    Serial.print(type);
    Serial.print(" -> ");
    Serial.println(message);
}
