/**
 * 文件：esp32_home_server/include/MqttUpstream.h
 * 功能说明：
 *   - 集中管理 MQTT 云端连接的配置参数
 *   - 提供统一的主题访问接口（传感器、状态、告警、控制）
 *   - 避免配置和主题字面量散落在各个模块中
 *
 * 核心接口（mqtt_upstream 命名空间）：
 *   - struct CloudConfig - 云端服务器配置（地址、端口、认证）
 *   - cloudConfig() - 获取云端配置
 *   - sensorTopic() - 获取传感器数据上报主题
 *   - statusTopic() - 获取系统状态上报主题
 *   - alarmTopic() - 获取告警上报主题
 *   - controlTopic() - 获取控制命令下行主题
 *
 * 被依赖于：ConnectivityManager.h, AutomationEngine.h
 * 被使用的主题前缀：esp32/home/<domain>
 *
 * 设计说明：
 *   - 使用命名空间避免全局污染
 *   - 所有配置集中在 MqttUpstream.cpp 中管理
 *   - 支持连接公网云服务器上的 EMQX 实例
 *   - 主题命名遵带 MQTT 最佳实践（分层结构）
 */

#ifndef MQTT_UPSTREAM_H
#define MQTT_UPSTREAM_H

#include <Arduino.h>

namespace mqtt_upstream
{
    /**
     * MQTT 云端配置结构体
     *
     * 功能：集中存储连接云端 MQTT 服务器所需的所有配置参数
     *
     * 成员说明：
     *   - host: MQTT 服务器地址（如 "117.72.207.199"），支持域名和 IP
     *   - port: MQTT 服务器端口（如 1885、8883）
     *   - clientId: MQTT 客户端标识（如 "esp32-home-server"），用于服务器区分设备
     *   - user: 连接用户名（MQTT 认证），若为 nullptr 则不使用用户名认证
     *   - password: 连接密码（MQTT 认证），若为 nullptr 则不使用密码认证
     */
    struct CloudConfig
    {
        const char *host;     // MQTT 服务器地址
        uint16_t port;        // MQTT 服务器端口
        const char *clientId; // MQTT 客户端 ID（设备标识）
        const char *user;     // MQTT 用户名（可空）
        const char *password; // MQTT 密码（可空）
    };

    /**
     * 获取云端 MQTT 基础配置
     *
     * 功能：返回配置的云端服务器连接参数（地址、端口、认证）
     *
     * 返回值：对 CloudConfig 结构体的常量引用
     *
     * 注意：返回引用避免结构体拷贝，提高效率
     */
    const CloudConfig &cloudConfig();

    /**
     * 获取传感器数据上报主题
     *
     * 功能：返回温湿度、光照、烟雾、雨滴等传感器数据的上报 MQTT 主题
     *
     * 返回值：主题字符串（如 "esp32/home/sensors"）
     */
    const char *sensorTopic();

    /**
     * 获取系统状态上报主题
     *
     * 功能：返回系统运行状态、执行器状态的上报 MQTT 主题
     *
     * 返回值：主题字符串（如 "esp32/home/status"）
     */
    const char *statusTopic();

    /**
     * 获取告警上报主题
     *
     * 功能：返回系统告警、故障报告的上报 MQTT 主题
     *
     * 返回值：主题字符串（如 "esp32/home/alarm"）
     */
    const char *alarmTopic();

    /**
     * 获取控制命令下行主题
     *
     * 功能：返回云端下发控制命令的 MQTT 主题
     *
     * 返回值：主题字符串（如 "esp32/home/control"）
     */
    const char *controlTopic();
} // 命名空间结束

#endif
