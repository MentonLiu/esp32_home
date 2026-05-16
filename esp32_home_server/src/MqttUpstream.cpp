/**
 * 文件：esp32_home_server/src/MqttUpstream.cpp
 * 功能说明：
 *   - 实现 MqttUpstream 命名空间中的配置访问函数
 *   - 管理静态的云端服务器配置和 MQTT 主题常量
 *   - 提供访问器函数供其他模块获取配置和主题
 *
 * 核心实现：
 *   - cloudConfig() - 返回云端服务器地址、端口、认证信息
 *   - sensorTopic/statusTopic/alarmTopic/controlTopic() - 返回各类主题字符串
 *
 * 配置详情：
 *   - 云端：FRP 隈道映射到 NAS 上的 EMQX（frp-era.com:10883）
 *   - 客户端 ID: esp32-home-server
 *   - 用户名: esp32_server
 *   - 主题前缀：esp32/home/
 *
 * 依赖：MqttUpstream.h
 * 被依赖于：ConnectivityManager.cpp, AutomationEngine.cpp
 *
 * 设计说明：
 *   - 所有配置字面量集中在此文件中，便于维护和修改
 *   - 使用静态 const 避免额外的堆内存分配
 *   - 主题命名遵带分层 MQTT 命名规范
 */

#include "MqttUpstream.h"

namespace
{
    // 云端 MQTT 服务器配置（集中管理）
    // 当前通过 FRP 隈道连接到 NAS 上的 EMQX 实例
    constexpr mqtt_upstream::CloudConfig kCloudConfig = {
        "frp-era.com",       // 云端服务器地址（FRP 隈道出口）
        10883,               // MQTT 端口（非标准 10883 是 FRP 映射的自定义端口）
        "esp32-home-server", // 客户端 ID（用于服务器识别本设备）
        "esp32_server",      // MQTT 用户名（认证凭证）
        "lbl450981"          // MQTT 密码（认证凭证）
    };

    // MQTT 主题常量定义
    // 命名约定：esp32/home/<功能领域>
    // 这样的分层结构便于主题订阅管理（如订阅 esp32/home/# 获取所有消息）
    constexpr const char *kSensorTopic = "esp32/home/sensors";  // 传感器数据上报
    constexpr const char *kStatusTopic = "esp32/home/status";   // 系统状态上报
    constexpr const char *kAlarmTopic = "esp32/home/alarm";     // 告警和故障上报
    constexpr const char *kControlTopic = "esp32/home/control"; // 云端控制命令下行
} // 命名空间结束

namespace mqtt_upstream
{
    const CloudConfig &cloudConfig()
    {
        // 返回常量引用避免配置结构体的拷贝，提高性能
        // 调用方获得的是对 kCloudConfig 的只读引用
        return kCloudConfig;
    }

    // 以下函数都是主题字符串的访问器
    // 好处：集中管理主题字符串，便于后续修改而无需遍历整个代码库

    const char *sensorTopic()
    {
        // 返回传感器数据上报主题（温湿度、光照、烟雾、雨滴）
        return kSensorTopic;
    }

    const char *statusTopic()
    {
        // 返回系统状态上报主题（执行器状态、运行模式、自动化状态等）
        return kStatusTopic;
    }

    const char *alarmTopic()
    {
        // 返回告警和故障上报主题（传感器故障、连接错误等）
        return kAlarmTopic;
    }

    const char *controlTopic()
    {
        // 返回控制命令下行主题（来自云端的设备控制指令）
        return kControlTopic;
    }
} // 命名空间结束
