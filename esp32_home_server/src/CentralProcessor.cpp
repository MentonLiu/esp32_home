// 文件说明：esp32_home_server/src/CentralProcessor.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "CentralProcessor.h"

#include "pins.h"
#include "Logger.h"

namespace
{
    // 当前项目固定的联网配置。
    constexpr const char *kStationSsid = "HW-2103";
    constexpr const char *kStationPassword = "20220715";
    constexpr const char *kApSsid = "esp32-server";
    constexpr const char *kApPassword = "lbl450981";
} // namespace

CentralProcessor::CentralProcessor()
    : sensorHub_(pins::DHT_DATA, pins::LIGHT_ANALOG, pins::MQ2_ANALOG, pins::FLAME_ANALOG),
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
    LOG_INFO("SYSTEM", "编译时间: %s %s", __DATE__, __TIME__);

    // 1) 先起传感器链路，保证后续状态接口有数据。
    LOG_INFO("INIT", "初始化传感器...");
    sensorDataProcessor_.begin();
    LOG_INFO("INIT", "传感器初始化完成");

    // 2) 初始化执行器。
    LOG_INFO("INIT", "初始化执行器...");
    commandProcessor_.begin();
    LOG_INFO("INIT", "执行器初始化完成");
    LOG_INFO("INIT", "电机 LEDC 通道: 5, 舵机: auto, 蜂鸣器 LEDC 通道: 6");

    // 3) 启动网络与网页服务。
    LOG_INFO("INIT", "初始化网络...");
    net_.begin();
    LOG_INFO("INIT", "网络初始化完成");

    // MQTT 方案尚未定稿，本轮先不启用实际链路。
    // net_.configureCloudMqtt(...);
    // net_.setMqttHandler(...);

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
    sensorDataProcessor_.shouldPublish();

    // 自动化联动依赖最新传感器数据。
    automationEngine_.loop(sensorDataProcessor_.latest());
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
