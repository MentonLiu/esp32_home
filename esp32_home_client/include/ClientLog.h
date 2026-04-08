#ifndef CLIENT_LOG_H
#define CLIENT_LOG_H

// 轻量级日志辅助函数与宏，供客户端各模块统一使用。

#include <Arduino.h>

#include <stdarg.h>
#include <stdio.h>

#include "ClientConfig.h"

namespace client_log
{
    // 保存启动时间戳，用于输出运行时长日志。
    inline unsigned long &bootMarkMs()
    {
        static unsigned long value = 0;
        return value;
    }

    // 标记 setup 启动时刻。
    inline void markBootStart()
    {
        bootMarkMs() = millis();
    }

    // 返回距启动标记点的毫秒数。
    inline unsigned long sinceBootMs()
    {
        return millis() - bootMarkMs();
    }

    // 根据配置判断当前日志级别是否启用。
    inline bool isEnabled(uint8_t level)
    {
        return client_config::kEnableDiagnosticsLog && level <= client_config::kDiagnosticsLogLevel;
    }

    // 通过间隔节流周期性日志或动作。
    inline bool allowPeriodic(unsigned long &lastMs, unsigned long intervalMs)
    {
        const unsigned long now = millis();
        if (now - lastMs < intervalMs)
        {
            return false;
        }
        lastMs = now;
        return true;
    }

    // 检测布尔状态变化，便于输出状态切换日志。
    inline bool changedBool(bool value, bool &lastValue)
    {
        if (value == lastValue)
        {
            return false;
        }
        lastValue = value;
        return true;
    }

    // 输出结构化单行日志。
    inline void logLine(const char *level,
                        const char *module,
                        const char *event,
                        const char *details)
    {
        Serial.printf("T=%lu U=%lu L=%s M=%s E=%s", millis(), sinceBootMs(), level, module, event);
        if (details != nullptr && details[0] != '\0')
        {
            Serial.print(' ');
            Serial.print(details);
        }
        Serial.println();
    }

    // 先格式化详情字符串，再转发到 logLine。
    inline void logFormat(const char *level,
                          const char *module,
                          const char *event,
                          const char *fmt,
                          ...)
    {
        char details[192];
        details[0] = '\0';

        if (fmt != nullptr && fmt[0] != '\0')
        {
            va_list args;
            va_start(args, fmt);
            vsnprintf(details, sizeof(details), fmt, args);
            va_end(args);
        }

        logLine(level, module, event, details);
    }
} // 日志命名空间

// 带级别判断的结构化日志宏。
#define CL_ERROR(module, event, details)                      \
    do                                                        \
    {                                                         \
        if (client_log::isEnabled(1))                         \
        {                                                     \
            client_log::logLine("E", module, event, details); \
        }                                                     \
    } while (0)

#define CL_WARN(module, event, details)                       \
    do                                                        \
    {                                                         \
        if (client_log::isEnabled(2))                         \
        {                                                     \
            client_log::logLine("W", module, event, details); \
        }                                                     \
    } while (0)

#define CL_INFO(module, event, details)                       \
    do                                                        \
    {                                                         \
        if (client_log::isEnabled(3))                         \
        {                                                     \
            client_log::logLine("I", module, event, details); \
        }                                                     \
    } while (0)

#define CL_DEBUG(module, event, details)                      \
    do                                                        \
    {                                                         \
        if (client_log::isEnabled(4))                         \
        {                                                     \
            client_log::logLine("D", module, event, details); \
        }                                                     \
    } while (0)

#define CL_ERRORF(module, event, fmt, ...)                                 \
    do                                                                     \
    {                                                                      \
        if (client_log::isEnabled(1))                                      \
        {                                                                  \
            client_log::logFormat("E", module, event, fmt, ##__VA_ARGS__); \
        }                                                                  \
    } while (0)

#define CL_WARNF(module, event, fmt, ...)                                  \
    do                                                                     \
    {                                                                      \
        if (client_log::isEnabled(2))                                      \
        {                                                                  \
            client_log::logFormat("W", module, event, fmt, ##__VA_ARGS__); \
        }                                                                  \
    } while (0)

#define CL_INFOF(module, event, fmt, ...)                                  \
    do                                                                     \
    {                                                                      \
        if (client_log::isEnabled(3))                                      \
        {                                                                  \
            client_log::logFormat("I", module, event, fmt, ##__VA_ARGS__); \
        }                                                                  \
    } while (0)

#define CL_DEBUGF(module, event, fmt, ...)                                 \
    do                                                                     \
    {                                                                      \
        if (client_log::isEnabled(4))                                      \
        {                                                                  \
            client_log::logFormat("D", module, event, fmt, ##__VA_ARGS__); \
        }                                                                  \
    } while (0)

#endif