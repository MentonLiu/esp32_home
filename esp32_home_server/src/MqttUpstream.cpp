#include "MqttUpstream.h"

namespace
{
    // 集中管理云端服务地址与主题字面量。
    constexpr mqtt_upstream::CloudConfig kCloudConfig = {
        "example.mosquitto.server",
        1883,
        "esp32-home-server",
        nullptr,
        nullptr};

    constexpr const char *kSensorTopic = "esp32/home/sensors";
    constexpr const char *kStatusTopic = "esp32/home/status";
    constexpr const char *kAlarmTopic = "esp32/home/alarm";
    constexpr const char *kControlTopic = "esp32/home/control";
} // 命名空间

namespace mqtt_upstream
{
    // 返回引用可避免配置结构体拷贝。
    const CloudConfig &cloudConfig()
    {
        return kCloudConfig;
    }

    const char *sensorTopic()
    {
        return kSensorTopic;
    }

    const char *statusTopic()
    {
        return kStatusTopic;
    }

    const char *alarmTopic()
    {
        return kAlarmTopic;
    }

    const char *controlTopic()
    {
        return kControlTopic;
    }
} // 命名空间结束
