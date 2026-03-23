// 文件说明：esp32_home_server/include/LocalProcessingProgram.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

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
