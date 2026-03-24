// 文件说明：esp32_home_server/include/Logger.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

// 日志输出工具类，提供统一的串口调试接口
class Logger
{
public:
    // 初始化串口（115200 波特率）
    static void begin();

    // 日志级别枚举
    enum Level
    {
        DEBUG = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3
    };

    // 输出日志（格式化字符串，支持 printf 风格参数）
    static void log(Level level, const char *tag, const char *format, ...);

    // 便捷宏函数
    static inline void debug(const char *tag, const char *format, ...)
    {
        va_list args;
        va_start(args, format);
        vlog(DEBUG, tag, format, args);
        va_end(args);
    }

    static inline void info(const char *tag, const char *format, ...)
    {
        va_list args;
        va_start(args, format);
        vlog(INFO, tag, format, args);
        va_end(args);
    }

    static inline void warn(const char *tag, const char *format, ...)
    {
        va_list args;
        va_start(args, format);
        vlog(WARN, tag, format, args);
        va_end(args);
    }

    static inline void error(const char *tag, const char *format, ...)
    {
        va_list args;
        va_start(args, format);
        vlog(ERROR, tag, format, args);
        va_end(args);
    }

private:
    static void vlog(Level level, const char *tag, const char *format, va_list args);
    static const char *levelName(Level level);
};

// 便捷全局日志宏定义
#define LOG_DEBUG(tag, fmt, ...) Logger::debug(tag, fmt, ##__VA_ARGS__)
#define LOG_INFO(tag, fmt, ...) Logger::info(tag, fmt, ##__VA_ARGS__)
#define LOG_WARN(tag, fmt, ...) Logger::warn(tag, fmt, ##__VA_ARGS__)
#define LOG_ERROR(tag, fmt, ...) Logger::error(tag, fmt, ##__VA_ARGS__)

#endif
