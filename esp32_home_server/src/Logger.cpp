// 文件说明：esp32_home_server/src/Logger.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "Logger.h"

#include <stdarg.h>

namespace
{
    // 日志缓冲区
    constexpr size_t kLogBufferSize = 256;
} // namespace

void Logger::begin()
{
    // 串口已在 CentralProcessor::begin() 中初始化
    // 这里仅用于确保日志系统已就绪
}

void Logger::vlog(Level level, const char *tag, const char *format, va_list args)
{
    // 格式：[时间] [级别] [标签] 消息
    char buffer[kLogBufferSize];
    unsigned long ms = millis();
    unsigned long sec = ms / 1000;
    unsigned long ms_part = ms % 1000;

    // 打印时间戳和级别
    snprintf(buffer, kLogBufferSize, "[%lu.%03lu] [%s] [%s] ", sec, ms_part, levelName(level), tag);
    Serial.print(buffer);

    // 打印格式化消息
    vsnprintf(buffer, kLogBufferSize, format, args);
    Serial.println(buffer);
}

const char *Logger::levelName(Level level)
{
    switch (level)
    {
    case DEBUG:
        return "DEBUG";
    case INFO:
        return "INFO";
    case WARN:
        return "WARN";
    case ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}
