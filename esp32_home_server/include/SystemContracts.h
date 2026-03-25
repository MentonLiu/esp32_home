// 文件说明：esp32_home_server/include/SystemContracts.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef SYSTEM_CONTRACTS_H
#define SYSTEM_CONTRACTS_H

#include <Arduino.h>
#include <functional>

// 系统运行模式：
// - Cloud: 连接家庭路由器，具备互联网访问能力。
// - LocalAP: 作为热点独立运行，供手机/电脑直连控制。
enum class OperatingMode : uint8_t
{
    Cloud = 0,
    LocalAP = 1
};

// 指令来源枚举，用于状态上报与审计。
enum class CommandSource : uint8_t
{
    LocalWeb = 0,
    CloudMqtt = 1,
    Automation = 2
};

// 风扇逻辑档位抽象，底层可映射到 PWM 百分比。
enum class FanMode : uint8_t
{
    Off = 0,
    Low = 1,
    Medium = 2,
    High = 3
};

// 标准化后的传感器数据结构：
// 本结构是“系统内部统一数据总线”，HTTP/MQTT 均从此派生。
struct StandardSensorData
{
    // 温度（摄氏度）。
    float temperatureC = 0.0F;
    // 相对湿度百分比。
    float humidityPercent = 0.0F;
    // 光照归一化百分比（0-100）。
    uint8_t lightPercent = 0;
    // MQ2 归一化百分比（0-100）。
    uint8_t mq2Percent = 0;
    // 雨滴归一化百分比（0-100）。
    uint8_t rainPercent = 0;
    // 是否处于下雨状态（由阈值与滞回策略计算）。
    bool isRaining = false;
    // 烟雾等级标签（green/blue/yellow/red）。
    String smokeLevel = "green";
    // 采样链路是否有错误。
    bool hasError = false;
    // 错误文字（如 dht_read_failed）。
    String errorMessage;
    // 采样时间戳（毫秒，来自 millis）。
    unsigned long timestamp = 0;
};

// 控制器聚合状态：
// 用于给前端页面和状态接口展示“当前执行器状态”。
struct ControllerState
{
    // 风扇档位（Off/Low/Medium/High）。
    FanMode fanMode = FanMode::Off;
    // 风扇速度百分比。
    uint8_t fanSpeedPercent = 0;
    // 窗帘当前角度（0-180）。
    uint8_t curtainAngle = 0;
    // 最近一次窗帘预设编号。
    uint8_t lastCurtainPreset = 0;
    // 是否存在有效预设（false 表示可能为手动角度）。
    bool hasCurtainPreset = false;
};

// 指令处理统一返回结果。
struct CommandResult
{
    // 是否被接收并执行。
    bool accepted = false;
    // 是否引起执行器状态变化。
    bool stateChanged = false;
    // 结果类型：local_command/cloud_command/automation/error 等。
    String type = "info";
    // 结果详情。
    String message;
};

// 状态上报函数类型，topic 可为空（仅本地打印场景）。
using StatusReporter = std::function<void(const char *topic, const String &type, const String &message)>;

inline const char *modeToString(OperatingMode mode)
{
    return mode == OperatingMode::Cloud ? "cloud" : "local_ap";
}

inline const char *fanModeToString(FanMode mode)
{
    // 保证 enum 到协议字符串映射集中管理。
    switch (mode)
    {
    case FanMode::Off:
        return "off";
    case FanMode::Low:
        return "low";
    case FanMode::Medium:
        return "medium";
    case FanMode::High:
        return "high";
    }

    return "off";
}

#endif
