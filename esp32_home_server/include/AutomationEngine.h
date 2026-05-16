/**
 * 文件：esp32_home_server/include/AutomationEngine.h
 * 功能说明：
 *   - 根据时间和传感器状态驱动执行器联动
 *   - 实现六大自动化规则：定时窗帘、温度风扇、烟雾告警、雨滴联动、光照调节、时间源管理
 *   - 提供手动覆盖机制：用户手动操作后暂时禁用自动化
 *   - 处理动作冷却期防止频繁切换
 *
 * 自动化规则详情：
 *   1. 定时窗帘：07:00 开启到预设档位，22:00 关闭
 *   2. 温度风扇：>28°C 自动高档，降至 <26°C 恢复低档
 *   3. 烟雾告警：>80% 高档风扇 + 周期鸣叫；>90% 加快鸣叫速率
 *   4. 雨滴联动：开始下雨立即关窗帘，停雨稳定 10 分钟后全开
 *   5. 光照调节：>50% 自动防眩（半开），<30% 自动补光（全开）
 *   6. 时间源：优先 NTP，失败回退到 Unix 基准 + millis()
 *
 * 依赖：ConnectivityManager.h, ControllerCommandProcessor.h, SystemContracts.h, Logger.h
 * 被依赖于：CentralProcessor.h, main.cpp
 *
 * 设计细节：
 *   - 手动覆盖期 30 分钟（防止自动化干扰）
 *   - 动作冷却期防止触发频率过高导致硬件磨损
 *   - 支持雨滴状态滞回避免边界抖动
 */

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
    void ensureTimeSource();
    // 统一当前时间来源：NTP -> Unix 回退时钟。
    time_t currentTime();
    // 每日定时窗帘规则。
    void handleCurtainSchedule(time_t nowUnix);
    // 温度联动风扇规则。
    void handleTemperatureAutomation(const StandardSensorData &sensorData);
    // 烟雾浓度联动规则。
    void handleSmokeAutomation(const StandardSensorData &sensorData);
    // 雨滴联动窗帘规则。
    void handleRainAutomation(const StandardSensorData &sensorData);
    // 光照联动窗帘规则。
    void handleLightAutomation(const StandardSensorData &sensorData, time_t nowUnix);
    // 统一状态/告警消息上报入口。
    void publishStatus(const char *topic, const String &type, const String &message) const;

    ConnectivityManager &net_;
    ControllerCommandProcessor &commandProcessor_;
    StatusReporter statusReporter_;

    // 是否已配置 NTP。
    bool ntpConfigured_ = false;
    // 回退时间基准（Unix 时间秒）。
    time_t fallbackBaseUnix_ = 0;
    unsigned long fallbackBaseMillis_ = 0;

    // 限流与去重状态。
    unsigned long lastScheduleCheckMs_ = 0;
    unsigned long lastHighSmokeBeepMs_ = 0;
    unsigned long rainDetectedSinceMs_ = 0;
    unsigned long rainClearedSinceMs_ = 0;
    unsigned long lastRainCurtainActionMs_ = 0;
    uint32_t lastOpenDayStamp_ = UINT32_MAX;
    uint32_t lastCloseDayStamp_ = UINT32_MAX;
    bool rainLockActive_ = false;

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
