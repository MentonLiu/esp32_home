#include "LocalProcessingProgram.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "MqttUpstream.h"

LocalProcessingProgram::LocalProcessingProgram(ConnectivityManager &net,
                                               SensorDataProcessor &sensorDataProcessor,
                                               ControllerCommandProcessor &commandProcessor,
                                               StatusReporter statusReporter)
    : net_(net),
      sensorDataProcessor_(sensorDataProcessor),
      commandProcessor_(commandProcessor),
      statusReporter_(statusReporter)
{
}

void LocalProcessingProgram::begin()
{
    // 挂载本地文件系统；即使失败也继续运行（控制接口仍可用）。
    fsReady_ = LittleFS.begin(true);
    setupRoutes();
    net_.webServer().begin();
}

void LocalProcessingProgram::setupRoutes()
{
    // 主控制面板页面。
    net_.webServer().on("/", HTTP_GET, [this]()
                        {
        if (!serveFile("/index.html", "text/html; charset=utf-8"))
        {
            net_.webServer().send(503, "text/plain", "index.html unavailable");
        } });

    // 项目架构说明页面。
    net_.webServer().on("/project-map", HTTP_GET, [this]()
                        {
        if (!serveFile("/project-map.html", "text/html; charset=utf-8"))
        {
            net_.webServer().send(503, "text/plain", "project-map.html unavailable");
        } });

    // 本地页面轮询的运行状态接口。
    net_.webServer().on("/api/status", HTTP_GET, [this]()
                        { net_.webServer().send(200,
                                                "application/json",
                                                sensorDataProcessor_.buildStatusJson(net_.mode(),
                                                                                     net_.ipString(),
                                                                                     commandProcessor_.state())); });

    // 本地控制接口：将载荷转发给统一命令处理器。
    net_.webServer().on("/api/control", HTTP_POST, [this]()
                        {
        const String requestBody = net_.webServer().arg("plain");
        const CommandResult result = commandProcessor_.processCommandJson(requestBody, CommandSource::LocalWeb);

        if (statusReporter_)
        {
            statusReporter_(mqtt_upstream::statusTopic(),
                            result.accepted ? result.type.c_str() : "error",
                            result.message);
        }

        JsonDocument response;
        response["ok"] = true;
        String payload;
        serializeJson(response, payload);
        net_.webServer().send(200, "application/json", payload); });
}

bool LocalProcessingProgram::serveFile(const char *path, const char *contentType)
{
    // 仅在文件系统可用时提供静态资源。
    if (!fsReady_)
    {
        return false;
    }

    File file = LittleFS.open(path, "r");
    if (!file)
    {
        return false;
    }

    net_.webServer().streamFile(file, contentType);
    file.close();
    return true;
}
