#ifndef MQTT_UPSTREAM_H
#define MQTT_UPSTREAM_H

#include <Arduino.h>

namespace mqtt_upstream
{
    struct CloudConfig
    {
        const char *host;
        uint16_t port;
        const char *clientId;
        const char *user;
        const char *password;
    };

    const CloudConfig &cloudConfig();
    const char *sensorTopic();
    const char *statusTopic();
    const char *alarmTopic();
    const char *controlTopic();
} // namespace mqtt_upstream

#endif
