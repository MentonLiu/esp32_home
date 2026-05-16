/**
 * 文件：esp32_home_server/src/LocalProcessingProgram.cpp
 * 功能说明：
 *   - 实现 HTTP 路由处理程序的注册
 *   - 实现静态网页文件服务（从 LittleFS 加载）
 *   - 实现 JSON API 端点（/api/status、/api/control）
 *   - 处理本地网络命令并转发给控制处理器
 *
 * 核心实现：
 *   - LocalProcessingProgram::begin() - 注册所有 HTTP 处理程序
 *   - LocalProcessingProgram::loop() - 驱动 WebServer 请求分发
 *   - handleStatus() - GET /api/status 实现
 *   - handleControl() - POST /api/control 实现
 *   - handleIndex() - 静态页面服务
 *
 * 依赖：LocalProcessingProgram.h, ConnectivityManager.h, Logger.h, LittleFS、ArduinoJson 库
 * 被依赖于：CentralProcessor.cpp, main.cpp
 *
 * 设计细节：
 *   - 所有 API 响应使用 JSON 格式
 *   - 网页首页来自 LittleFS:/index.html
 *   - 本地命令来源统一标记为 LocalWeb（用于审计）
 *   - 支持 CORS 预检请求（可选）
 */

#include "LocalProcessingProgram.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "Logger.h"
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
    // 挂载 LittleFS：true 表示失败时尝试格式化恢复。
    fsReady_ = LittleFS.begin(true);
    // 注册 HTTP 路由。
    setupRoutes();
    // 启动内置 WebServer。
    net_.webServer().begin();
}

void LocalProcessingProgram::setupRoutes()
{
    // 首页：返回 LittleFS 中的控制页面。
    net_.webServer().on("/", HTTP_GET, [this]()
                        {
        net_.webServer().sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
        net_.webServer().sendHeader("Pragma", "no-cache");
        net_.webServer().sendHeader("Expires", "0");
        if (!serveFile("/index.html", "text/html; charset=utf-8"))
        {
            net_.webServer().send(503, "text/plain", "index.html unavailable");
        } });

    // 状态接口：返回完整系统状态 JSON。
    net_.webServer().on("/api/status", HTTP_GET, [this]()
                        {
        net_.webServer().sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
        net_.webServer().sendHeader("Pragma", "no-cache");
        net_.webServer().sendHeader("Expires", "0");
        net_.webServer().send(200,
                                                "application/json",
                                                sensorDataProcessor_.buildStatusJson(net_.mode(),
                                                                                     net_.ipString(),
                                                                                     commandProcessor_.state())); });

    // 控制接口：接收 JSON 命令并反馈执行结果。
    net_.webServer().on("/api/control", HTTP_POST, [this]()
                        {
        // WebServer 的 plain 参数中保存原始请求体。
        const String requestBody = net_.webServer().arg("plain");
        const CommandResult result = commandProcessor_.processCommandJson(requestBody, CommandSource::LocalWeb);

        if (!result.accepted)
        {
            LOG_WARN("HTTP", "control rejected: %s, body=%s", result.message.c_str(), requestBody.c_str());
        }

        if (statusReporter_)
        {
            // 把执行结果同步到状态上报通道。
            statusReporter_(mqtt_upstream::statusTopic(),
                            result.accepted ? result.type.c_str() : "error",
                            result.message);
        }

        // 统一返回结果结构，便于前端处理。
        JsonDocument response;
        response["ok"] = result.accepted;
        response["stateChanged"] = result.stateChanged;
        response["type"] = result.type;
        response["message"] = result.message;

        String payload;
        serializeJson(response, payload);
        net_.webServer().send(result.accepted ? 200 : 400, "application/json", payload); });
}

bool LocalProcessingProgram::serveFile(const char *path, const char *contentType)
{
    if (!fsReady_)
    {
        // 文件系统不可用时直接失败。
        return false;
    }

    File file = LittleFS.open(path, "r");
    if (!file)
    {
        // 文件不存在或打开失败。
        return false;
    }

    // 通过流式传输降低内存压力。
    net_.webServer().streamFile(file, contentType);
    file.close();
    return true;
}
