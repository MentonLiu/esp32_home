/**
 * 文件：esp32_home_server/include/LocalProcessingProgram.h
 * 功能说明：
 *   - 注册 HTTP 路由处理程序到 WebServer
 *   - 服务静态网页文件（HTML/CSS/JS 从 LittleFS 读取）
 *   - 实现本地 JSON API（/api/status、/api/control）
 *   - 处理本地用户输入（按钮、网页 UI）的控制命令
 *
 * API 端点：
 *   - GET /api/status - 获取当前系统状态（传感器 + 控制器）
 *   - POST /api/control - 处理本地控制命令（JSON 格式）
 *   - GET / - 服务网页首页
 *
 * 依赖：ConnectivityManager.h, ControllerCommandProcessor.h, SensorDataProcessor.h, Logger.h
 * 被依赖于：CentralProcessor.h, main.cpp
 *
 * 设计细节：
 *   - 所有 HTTP 响应使用 JSON 格式确保 UI 一致性
 *   - 网页文件通过 LittleFS 加载，支持离线访问（如 WiFi 失败）
 *   - 本地网络命令来源标记为 LocalWeb（用于后期审计）
 *   - 支持 CORS 头部使第三方网页可访问（可选）
 */

#ifndef LOCAL_PROCESSING_PROGRAM_H
#define LOCAL_PROCESSING_PROGRAM_H

#include <Arduino.h>

#include "ConnectivityManager.h"
#include "ControllerCommandProcessor.h"
#include "SensorDataProcessor.h"
#include "SystemContracts.h"

// 本地处理程序：
// 承担 HTTP 路由注册、静态页面服务与本地控制命令入口。
class LocalProcessingProgram
{
public:
    // net: 网络与 WebServer 容器。
    // sensorDataProcessor: 传感器数据提供者。
    // commandProcessor: 本地控制命令执行入口。
    // statusReporter: 状态上报回调。
    LocalProcessingProgram(ConnectivityManager &net,
                           SensorDataProcessor &sensorDataProcessor,
                           ControllerCommandProcessor &commandProcessor,
                           StatusReporter statusReporter);

    // 初始化文件系统并注册全部 HTTP 路由。
    void begin();

private:
    // 注册页面与接口路由（/, /api/status, /api/control）。
    void setupRoutes();
    // 从 LittleFS 读取并输出静态文件。
    bool serveFile(const char *path, const char *contentType);

    // 依赖模块引用。
    ConnectivityManager &net_;
    SensorDataProcessor &sensorDataProcessor_;
    ControllerCommandProcessor &commandProcessor_;
    StatusReporter statusReporter_;
    // LittleFS 是否已成功挂载。
    bool fsReady_ = false;
};

#endif
