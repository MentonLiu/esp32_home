// 文件说明：esp32_home_server/src/MqttUpstream.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "MqttUpstream.h"

namespace
{
    // 集中管理云端服务地址与主题字面量。
    // 当前使用 NAS 上的 EMQX 映射地址。
    constexpr mqtt_upstream::CloudConfig kCloudConfig = {
        "frp-era.com",
        10883,
        "esp32-home-server",
        "esp32_server",
        "lbl450981"};

    // 主题命名约定：esp32/home/<domain>
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

    // 以下访问器函数统一返回静态主题常量。
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
