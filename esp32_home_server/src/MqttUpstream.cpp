#include "MqttUpstream.h"

namespace
{
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
} // namespace

namespace mqtt_upstream
{
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
} // namespace mqtt_upstream
