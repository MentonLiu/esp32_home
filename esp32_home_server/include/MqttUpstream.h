#ifndef MQTT_UPSTREAM_H
#define MQTT_UPSTREAM_H

#include <Arduino.h>

namespace mqtt_upstream
{
    // 用于上行遥测与控制的静态云端消息配置。
    struct CloudConfig
    {
        const char *host;
        uint16_t port;
        const char *clientId;
        const char *user;
        const char *password;
    };

    // 通过访问器集中管理主题和配置字面量，避免散落在各处。
    const CloudConfig &cloudConfig();
    const char *sensorTopic();
    const char *statusTopic();
    const char *alarmTopic();
    const char *controlTopic();
} // 命名空间结束

#endif
