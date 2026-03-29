#ifndef CLIENT_LOG_H
#define CLIENT_LOG_H

#include <Arduino.h>

#include <stdarg.h>
#include <stdio.h>

#include "ClientConfig.h"

namespace client_log
{
    inline unsigned long &bootMarkMs()
    {
        static unsigned long value = 0;
        return value;
    }

    inline void markBootStart()
    {
        bootMarkMs() = millis();
    }

    inline unsigned long sinceBootMs()
    {
        return millis() - bootMarkMs();
    }

    inline bool isEnabled(uint8_t level)
    {
        return client_config::kEnableDiagnosticsLog && level <= client_config::kDiagnosticsLogLevel;
    }

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

    inline bool changedBool(bool value, bool &lastValue)
    {
        if (value == lastValue)
        {
            return false;
        }
        lastValue = value;
        return true;
    }

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
} // namespace client_log

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