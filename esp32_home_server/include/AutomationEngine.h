#ifndef AUTOMATION_ENGINE_H
#define AUTOMATION_ENGINE_H

#include <Arduino.h>
#include <RTClib.h>

#include "ConnectivityManager.h"
#include "ControllerCommandProcessor.h"
#include "SystemContracts.h"

// 自动化策略引擎：处理定时与安全事件触发逻辑。
class AutomationEngine
{
public:
    AutomationEngine(ConnectivityManager &net,
                     ControllerCommandProcessor &commandProcessor,
                     StatusReporter statusReporter);

    void begin();
    void loop(const StandardSensorData &sensorData);

private:
    // 网络可用后启用网络授时。
    void ensureTimeSource();
    // 通过网络授时对实时时钟进行一次性校时。
    void syncRtcFromNtp();
    // 返回当前最可靠的本地时间。
    DateTime currentTime();
    // 处理窗帘定时开合。
    void handleCurtainSchedule(const DateTime &now);
    // 烟雾相关的风扇和蜂鸣器策略。
    void handleSmokeAutomation(const StandardSensorData &sensorData);
    // 火焰持续时长升级与云端告警策略。
    void handleFlameAutomation(const StandardSensorData &sensorData);
    void publishStatus(const char *topic, const String &type, const String &message) const;

    ConnectivityManager &net_;
    ControllerCommandProcessor &commandProcessor_;
    StatusReporter statusReporter_;

    RTC_DS3231 rtc_;
    bool rtcAvailable_ = false;
    bool ntpConfigured_ = false;
    bool rtcSyncedFromNtp_ = false;
    DateTime fallbackBaseTime_;
    unsigned long fallbackBaseMillis_ = 0;

    // 内部计时与去重状态。
    unsigned long lastScheduleCheckMs_ = 0;
    unsigned long flameDetectedSinceMs_ = 0;
    unsigned long lastHighSmokeBeepMs_ = 0;
    unsigned long lastFlamePatternMs_ = 0;
    uint32_t lastOpenDayStamp_ = UINT32_MAX;
    uint32_t lastCloseDayStamp_ = UINT32_MAX;
    bool fireAlarmReported_ = false;
};

#endif
