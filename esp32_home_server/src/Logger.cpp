/**
 * 文件：esp32_home_server/src/Logger.cpp
 * 功能说明：
 *   - 实现 Logger 类的日志输出功能
 *   - 处理日志格式化、时间戳添加、串口输出
 *   - 提供日志级别名称转换
 *
 * 核心实现：
 *   - Logger::begin() - 日志系统初始化
 *   - Logger::vlog() - 格式化日志输出到串口
 *   - Logger::levelName() - 日志级别枚举转字符串
 *
 * 依赖：Logger.h, stdarg.h
 * 被依赖于：几乎所有模块
 *
 * 设计细节：
 *   - 日志缓冲区 256 字节，足以容纳大多数日志消息
 *   - 时间戳精确到秒级，使用 millis() 转换
 *   - 输出格式统一为 [HH:MM:SS] [级别] [标签] 消息
 *   - 串口 115200 波特率已在系统启动时初始化
 */

#include "Logger.h"

#include <stdarg.h>

namespace
{
    // 日志缓冲区大小（字节）
    // 每条日志包括：时间戳 + 级别 + 标签 + 消息，256 字节足以容纳
    constexpr size_t kLogBufferSize = 256;
} // namespace

void Logger::begin()
{
    // 串口已在 CentralProcessor::begin() 中初始化，波特率 115200
    // 这里仅用于确保日志系统已就绪，如有需要可添加额外初始化
}

void Logger::vlog(Level level, const char *tag, const char *format, va_list args)
{
    // 构建日志消息缓冲区
    char buffer[kLogBufferSize];

    // 计算当前运行时间（从 millis() 转换为 HH:MM:SS 格式）
    unsigned long ms = millis();
    unsigned long sec = ms / 1000;          // 转换为秒
    unsigned long hour = (sec / 3600) % 24; // 小时（0-23）
    unsigned long minute = (sec / 60) % 60; // 分钟（0-59）
    unsigned long second = sec % 60;        // 秒（0-59）

    // 第一步：打印时间戳和日志头信息（级别、标签）
    // 格式：[HH:MM:SS] [级别] [标签]
    snprintf(buffer, kLogBufferSize, "[%02lu:%02lu:%02lu] [%s] [%s] ",
             hour, minute, second, levelName(level), tag);
    Serial.print(buffer);

    // 第二步：打印格式化的日志内容消息
    vsnprintf(buffer, kLogBufferSize, format, args);
    Serial.println(buffer); // println 自动添加换行符
}

const char *Logger::levelName(Level level)
{
    // 将日志级别枚举转换为对应的字符串表示
    // 用于在日志输出中显示可读的级别名称
    switch (level)
    {
    case DEBUG:
        return "DEBUG"; // 调试信息
    case INFO:
        return "INFO"; // 信息消息
    case WARN:
        return "WARN"; // 警告消息
    case ERROR:
        return "ERROR"; // 错误消息
    default:
        return "UNKNOWN"; // 未知级别（应不会发筯）
    }
}
