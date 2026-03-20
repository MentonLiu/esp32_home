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
