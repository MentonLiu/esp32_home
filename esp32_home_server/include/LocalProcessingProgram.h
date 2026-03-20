#ifndef LOCAL_PROCESSING_PROGRAM_H
#define LOCAL_PROCESSING_PROGRAM_H

#include <Arduino.h>

#include "ConnectivityManager.h"
#include "ControllerCommandProcessor.h"
#include "SensorDataProcessor.h"
#include "SystemContracts.h"

// 本地热点模式下的网页应用宿主与接口路由器。
class LocalProcessingProgram
{
public:
    LocalProcessingProgram(ConnectivityManager &net,
                           SensorDataProcessor &sensorDataProcessor,
                           ControllerCommandProcessor &commandProcessor,
                           StatusReporter statusReporter);

    void begin();

private:
    // 注册静态页面与控制端点。
    void setupRoutes();
    // 从本地文件系统流式输出单个文件到网页响应。
    bool serveFile(const char *path, const char *contentType);

    ConnectivityManager &net_;
    SensorDataProcessor &sensorDataProcessor_;
    ControllerCommandProcessor &commandProcessor_;
    StatusReporter statusReporter_;
    bool fsReady_ = false;
};

#endif
