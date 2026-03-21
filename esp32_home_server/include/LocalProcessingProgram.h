// 文件说明：esp32_home_server/include/LocalProcessingProgram.h
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#ifndef LOCAL_PROCESSING_PROGRAM_H
#define LOCAL_PROCESSING_PROGRAM_H

#include <Arduino.h>

#include "ConnectivityManager.h"
#include "ControllerCommandProcessor.h"
#include "SensorDataProcessor.h"
#include "SystemContracts.h"

class LocalProcessingProgram
{
public:
    LocalProcessingProgram(ConnectivityManager &net,
                           SensorDataProcessor &sensorDataProcessor,
                           ControllerCommandProcessor &commandProcessor,
                           StatusReporter statusReporter);

    void begin();

private:
    void setupRoutes();
    bool serveFile(const char *path, const char *contentType);

    ConnectivityManager &net_;
    SensorDataProcessor &sensorDataProcessor_;
    ControllerCommandProcessor &commandProcessor_;
    StatusReporter statusReporter_;
    bool fsReady_ = false;
};

#endif
