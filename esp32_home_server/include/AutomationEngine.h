// 文件说明：esp32_home_server/include/AutomationEngine.h
// 自动化引擎模块声明，负责按时间与传感器状态驱动执行器联动。
//
// 已实现的自动化操作：
// 1) 每日定时窗帘：07:00 自动开到预设档位，22:00 自动关闭。
// 2) 温度联动风扇：高温自动切高档，温度回落后退出高温增强状态。
// 3) 烟雾联动：烟雾达到阈值自动开高档风扇，并在高危区间周期性蜂鸣提醒。
// 4) 光照联动窗帘：白天按亮度自动防眩/补光，带手动优先与动作冷却机制。
// 5) 时间源管理：优先使用 NTP，失败时回退到编译时刻+运行时长，保障离线定时可用。

#ifndef AUTOMATION_ENGINE_H
#define AUTOMATION_ENGINE_H

#include <Arduino.h>
#include <time.h>

#include "ConnectivityManager.h"
#include "ControllerCommandProcessor.h"
#include "SystemContracts.h"

// 自动化引擎：
// 聚合时间调度、烟雾联动等规则，不直接关心 UI 或协议细节。
class AutomationEngine
{
public:
    AutomationEngine(ConnectivityManager &net,
                     ControllerCommandProcessor &commandProcessor,
                     StatusReporter statusReporter);

    // 初始化时间源（NTP）与自动化基础状态。
    void begin();
    // 周期性执行自动化规则。
    void loop(const StandardSensorData &sensorData);

private:
    // 联网后配置 NTP 时间服务。
    void ensureTimeSource();
    // 统一当前时间来源：NTP -> 编译时回退时钟。
    time_t currentTime();
    // 每日定时窗帘规则。
    void handleCurtainSchedule(time_t nowEpoch);
    // 温度联动风扇规则。
    void handleTemperatureAutomation(const StandardSensorData &sensorData);
    // 烟雾浓度联动规则。
    void handleSmokeAutomation(const StandardSensorData &sensorData);
    // 光照联动窗帘规则。
    void handleLightAutomation(const StandardSensorData &sensorData, time_t nowEpoch);
    // 统一状态/告警消息上报入口。
    void publishStatus(const char *topic, const String &type, const String &message) const;

    ConnectivityManager &net_;
    ControllerCommandProcessor &commandProcessor_;
    StatusReporter statusReporter_;

    // 是否已配置 NTP。
    bool ntpConfigured_ = false;
    // 回退时间基准：当 NTP 不可用时使用。
    time_t fallbackBaseEpoch_ = 0;
    unsigned long fallbackBaseMillis_ = 0;

    // 限流与去重状态。
    unsigned long lastScheduleCheckMs_ = 0;
    unsigned long lastHighSmokeBeepMs_ = 0;
    uint32_t lastOpenDayStamp_ = UINT32_MAX;
    uint32_t lastCloseDayStamp_ = UINT32_MAX;

    bool tempFanBoostActive_ = false;
    unsigned long lastTempFanActionMs_ = 0;

    enum class LightCurtainMode : uint8_t
    {
        Neutral = 0,
        AntiGlare = 1,
        DaylightBoost = 2
    };
    LightCurtainMode lightCurtainMode_ = LightCurtainMode::Neutral;
    unsigned long lastLightCurtainActionMs_ = 0;
};

#endif
