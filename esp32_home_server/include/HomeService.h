/**
 * @file HomeService.h
 * @brief 家庭智能服务头文件
 * 
 * 核心服务类，整合所有传感器和执行器，提供：
 * - 传感器数据采集与发布
 * - 自动化控制逻辑(定时开关窗帘)
 * - 报警处理(烟雾、火焰检测)
 * - Web控制界面
 * - MQTT远程控制
 */

#ifndef HOME_SERVICE_H
#define HOME_SERVICE_H

#include <Arduino.h>

#include "ConnectivityManager.h"
#include "Controllerr.h"
#include "Sensor.h"

/**
 * @brief 家庭智能服务主类
 * @details 协调所有子系统，提供完整的智能家居功能
 */
class HomeService
{
public:
    /**
     * @brief 构造函数
     * 初始化所有子系统的实例
     */
    HomeService();

    /**
     * @brief 系统初始化
     * 初始化所有传感器、控制器和网络连接
     */
    void begin();

    /**
     * @brief 主循环
     * 在Arduino loop()中调用，处理所有业务逻辑
     */
    void loop();

private:
    /**
     * @brief 配置Web服务器路由
     * 设置HTTP请求处理函数
     */
    void setupWebRoutes();

    /**
     * @brief 处理控制命令
     * @param jsonText JSON格式的控制命令
     */
    void processControlCommand(const String &jsonText);

    /**
     * @brief 处理传感器数据并发布
     * 定期读取传感器数据并通过MQTT发布
     */
    void handleSensorAndPublish();

    /**
     * @brief 处理自动化任务
     * 包括定时开关窗帘等功能
     */
    void handleAutomation();

    /**
     * @brief 处理报警逻辑
     * 检测异常并触发报警动作
     */
    void handleAlerts();

    /**
     * @brief 发布状态消息
     * @param topic MQTT主题
     * @param type 消息类型
     * @param message 消息内容
     */
    void publishStatus(const char *topic, const String &type, const String &message) const;

    /**
     * @brief 构建状态JSON
     * @param includeMeta 是否包含IP、模式等元信息
     * @return String JSON字符串
     */
    String buildStatusJson(bool includeMeta = true) const;

    /**
     * @brief 获取当前小时
     * @return int 当前小时(0-23)
     */
    int currentHour() const;

    /**
     * @brief 获取当前分钟
     * @return int 当前分钟(0-59)
     */
    int currentMinute() const;

    /**
     * @brief 获取当前天索引
     * @return uint32_t 从1970年算起的天数
     */
    uint32_t currentDayIndex() const;

    // 子系统组件
    SensorHub sensors_;                 ///< 传感器集线器
    RelayFanController fan_;            ///< 风扇控制器
    DualCurtainController curtain_;      ///< 窗帘控制器
    BuzzerController buzzer_;           ///< 蜂鸣器控制器
    IRController ir_;                   ///< 红外控制器
    ConnectivityManager net_;            ///< 网络管理器

    // 状态变量
    unsigned long lastSensorPublishMs_ = 0;    ///< 上次传感器发布
    unsigned long lastAutomationMs_ = 0;       ///< 上次自动化执行
    unsigned long flameDetectedSinceMs_ = 0;   ///< 火焰检测开始时间
    unsigned long lastHighSmokeBeepMs_ = 0;    ///< 上次烟雾警报蜂鸣
    unsigned long lastFlamePatternMs_ = 0;     ///< 上次火焰警报模式

    uint32_t lastOpenDay_ = UINT32_MAX;      ///< 上次窗帘打开日期
    uint32_t lastCloseDay_ = UINT32_MAX;     ///< 上次窗帘关闭日期
    bool fireAlarmReported_ = false;          ///< 火警是否已上报

    uint32_t bootBaseSeconds_ = 0;            ///< 启动时间基准(无NTP时使用)
    bool ntpConfigured_ = false;              ///< NTP是否已配置
};

#endif
