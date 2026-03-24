// 文件说明：esp32_home_server/include/AutomationEngine.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef AUTOMATION_ENGINE_H
#define AUTOMATION_ENGINE_H

#include <Arduino.h>
#include <RTClib.h>

#include "ConnectivityManager.h"
#include "ControllerCommandProcessor.h"
#include "SystemContracts.h"

// 自动化引擎：
// 聚合时间调度、烟雾联动、火焰联动等规则，不直接关心 UI 或协议细节。
class AutomationEngine
{
public:
    AutomationEngine(ConnectivityManager &net,
                     ControllerCommandProcessor &commandProcessor,
                     StatusReporter statusReporter);

    // 初始化时间源（NTP/RTC）与自动化基础状态。
    void begin();
    // 周期性执行自动化规则。
    void loop(const StandardSensorData &sensorData);

private:
    // 联网后配置 NTP 时间服务。
    void ensureTimeSource();
    // 在 NTP 可用时将时间同步到 RTC，提升离线可用性。
    void syncRtcFromNtp();
    // 统一当前时间来源：NTP -> RTC -> 编译时回退时钟。
    DateTime currentTime();
    // 每日定时窗帘规则。
    void handleCurtainSchedule(const DateTime &now);
    // 温度联动风扇规则。
    void handleTemperatureAutomation(const StandardSensorData &sensorData);
    // 烟雾浓度联动规则。
    void handleSmokeAutomation(const StandardSensorData &sensorData);
    // 光照联动窗帘规则。
    void handleLightAutomation(const StandardSensorData &sensorData, const DateTime &now);
    // 火焰告警联动规则。
    void handleFlameAutomation(const StandardSensorData &sensorData);
    // 统一状态/告警消息上报入口。
    void publishStatus(const char *topic, const String &type, const String &message) const;

    ConnectivityManager &net_;
    ControllerCommandProcessor &commandProcessor_;
    StatusReporter statusReporter_;

    // DS3231 RTC 实例（I2C）。
    RTC_DS3231 rtc_;
    // RTC 硬件是否可用。
    bool rtcAvailable_ = false;
    // 是否已配置 NTP。
    bool ntpConfigured_ = false;
    // 是否已完成 NTP -> RTC 一次同步。
    bool rtcSyncedFromNtp_ = false;
    // 回退时间基准：当 NTP 与 RTC 都不可用时使用。
    DateTime fallbackBaseTime_;
    unsigned long fallbackBaseMillis_ = 0;

    // 限流与去重状态。
    unsigned long lastScheduleCheckMs_ = 0;
    unsigned long flameDetectedSinceMs_ = 0;
    unsigned long lastHighSmokeBeepMs_ = 0;
    unsigned long lastFlamePatternMs_ = 0;
    uint32_t lastOpenDayStamp_ = UINT32_MAX;
    uint32_t lastCloseDayStamp_ = UINT32_MAX;
    bool fireAlarmReported_ = false;

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
