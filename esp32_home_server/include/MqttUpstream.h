// 文件说明：esp32_home_server/include/MqttUpstream.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef MQTT_UPSTREAM_H
#define MQTT_UPSTREAM_H

#include <Arduino.h>

namespace mqtt_upstream
{
    // 用于上行遥测与控制的静态云端消息配置。
    struct CloudConfig
    {
        // MQTT 服务器地址。
        const char *host;
        // MQTT 服务器端口。
        uint16_t port;
        // 客户端 ID。
        const char *clientId;
        // 用户名（可空）。
        const char *user;
        // 密码（可空）。
        const char *password;
    };

    // 通过访问器集中管理主题和配置字面量，避免散落在各处。
    // 获取云端 MQTT 基础配置。
    const CloudConfig &cloudConfig();
    // 传感器遥测主题。
    const char *sensorTopic();
    // 系统状态主题。
    const char *statusTopic();
    // 告警主题。
    const char *alarmTopic();
    // 控制下行主题。
    const char *controlTopic();
} // 命名空间结束

#endif
