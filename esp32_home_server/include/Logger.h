/**
 * 文件：esp32_home_server/include/Logger.h
 * 功能说明：
 *   - 提供统一的日志输出接口，封装串口调试功能
 *   - 支持多级日志（DEBUG/INFO/WARN/ERROR）
 *   - 提供 printf 风格的格式化字符串输出
 *   - 自动添加时间戳、日志级别和标签前缀
 *
 * 核心接口：
 *   - Logger::begin() - 初始化日志系统（串口已由 CentralProcessor 初始化）
 *   - Logger::log() - 通用日志输出方法
 *   - Logger::debug/info/warn/error() - 便捷级别方法
 *   - 宏定义 LOG_DEBUG/LOG_INFO/LOG_WARN/LOG_ERROR - 推荐使用的日志宏
 *
 * 被依赖于：几乎所有模块都会使用日志
 * 依赖：Arduino.h（标准库）
 *
 * 设计说明：
 *   - 使用静态成员函数确保全局单例
 *   - 日志缓冲区大小 256 字节，适合嵌入式环境
 *   - 时间戳使用 millis()，精确到秒级
 *   - 所有输出级别均通过串口同步打印
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

/**
 * 日志输出工具类
 *
 * 功能：提供统一的日志输出接口，支持多级别日志输出
 *
 * 使用示例：
 *   Logger::begin();
 *   Logger::info("App", "System starting...");
 *   LOG_ERROR("Motor", "Speed out of range: %d", speed);
 */
class Logger
{
public:
    /**
     * 初始化日志系统
     *
     * 功能：确保日志系统已就绪。注意：串口已在 CentralProcessor::begin() 中初始化
     *
     * 返回值：无
     */
    static void begin();

    /**
     * 日志级别枚举
     *
     * DEBUG (0) - 调试信息
     * INFO (1)  - 信息消息
     * WARN (2)  - 警告消息
     * ERROR (3) - 错误消息
     */
    enum Level
    {
        DEBUG = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3
    };

    /**
     * 输出日志（核心方法）
     *
     * 功能：根据指定的级别、标签和格式字符串输出日志
     * 输出格式：[HH:MM:SS] [级别] [标签] 消息
     *
     * 参数：
     *   - level: 日志级别（DEBUG/INFO/WARN/ERROR）
     *   - tag: 日志标签（如 "Motor", "WiFi"，用于识别日志来源）
     *   - format: 格式化字符串（支持 printf 风格，如 "%d", "%s"）
     *   - ...: 可变参数列表
     *
     * 返回值：无
     *
     * 注意：输出通过串口同步进行，不会阻塞（串口缓冲）
     */
    static void log(Level level, const char *tag, const char *format, ...);

    /**
     * DEBUG 级别日志便捷方法
     *
     * 等同于 log(DEBUG, tag, format, ...)
     */
    static inline void debug(const char *tag, const char *format, ...)
    {
        va_list args;
        va_start(args, format);
        vlog(DEBUG, tag, format, args);
        va_end(args);
    }

    /**
     * INFO 级别日志便捷方法
     *
     * 等同于 log(INFO, tag, format, ...)
     */
    static inline void info(const char *tag, const char *format, ...)
    {
        va_list args;
        va_start(args, format);
        vlog(INFO, tag, format, args);
        va_end(args);
    }

    /**
     * WARN 级别日志便捷方法
     *
     * 等同于 log(WARN, tag, format, ...)
     */
    static inline void warn(const char *tag, const char *format, ...)
    {
        va_list args;
        va_start(args, format);
        vlog(WARN, tag, format, args);
        va_end(args);
    }

    /**
     * ERROR 级别日志便捷方法
     *
     * 等同于 log(ERROR, tag, format, ...)
     */
    static inline void error(const char *tag, const char *format, ...)
    {
        va_list args;
        va_start(args, format);
        vlog(ERROR, tag, format, args);
        va_end(args);
    }

private:
    /**
     * 内部日志实现（变参版本）
     *
     * 功能：处理变参日志的核心逻辑，输出完整的日志行
     *
     * 参数：
     *   - level: 日志级别
     *   - tag: 日志标签
     *   - format: 格式化字符串
     *   - args: va_list 参数列表
     */
    static void vlog(Level level, const char *tag, const char *format, va_list args);

    /**
     * 日志级别转字符串
     *
     * 功能：将 Level 枚举转换为字符串表示（"DEBUG", "INFO" 等）
     *
     * 参数：
     *   - level: 日志级别
     *
     * 返回值：级别对应的字符串（如 "ERROR"）
     */
    static const char *levelName(Level level);
};

/**
 * 日志便捷宏定义
 *
 * 这些宏提供了更简洁的日志调用方式，推荐在代码中使用这些宏而不是直接调用 Logger::*
 *
 * 使用示例：
 *   LOG_INFO("App", "Temperature: %.1f°C", temp);
 *   LOG_ERROR("Motor", "Failed to initialize at GPIO %d", pin);
 */
#define LOG_DEBUG(tag, fmt, ...) Logger::debug(tag, fmt, ##__VA_ARGS__) // 调试日志
#define LOG_INFO(tag, fmt, ...) Logger::info(tag, fmt, ##__VA_ARGS__)   // 信息日志
#define LOG_WARN(tag, fmt, ...) Logger::warn(tag, fmt, ##__VA_ARGS__)   // 警告日志
#define LOG_ERROR(tag, fmt, ...) Logger::error(tag, fmt, ##__VA_ARGS__) // 错误日志

#endif
