/**
 * @file HomeServiceControl.cpp
 * @brief HomeService 的 Web、控制与状态发布实现
 */

#include "HomeService.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

namespace
{
    const char *kSensorTopic = "esp32/home/sensors";
    const char *kStatusTopic = "esp32/home/status";
}

void HomeService::setupWebRoutes()
{
    static bool fsReady = false;
    if (!fsReady)
    {
        fsReady = LittleFS.begin(true);
        if (!fsReady)
        {
            Serial.println("LittleFS mount failed");
            publishStatus(kStatusTopic, "error", "littlefs_mount_failed");
        }
    }

    net_.webServer().on("/", HTTP_GET, [this]()
                        {
        if (!fsReady)
        {
                net_.webServer().send(503, "text/plain", "LittleFS unavailable");
                return;
        }

        File indexFile = LittleFS.open("/index.html", "r");
        if (!indexFile)
        {
                net_.webServer().send(404, "text/plain", "index.html not found in LittleFS");
                return;
        }

        net_.webServer().streamFile(indexFile, "text/html; charset=utf-8");
        indexFile.close(); });

    net_.webServer().on("/api/status", HTTP_GET, [this]()
                        { net_.webServer().send(200, "application/json", buildStatusJson(true)); });

    net_.webServer().on("/api/control", HTTP_POST, [this]()
                        {
    processControlCommand(net_.webServer().arg("plain"));
    net_.webServer().send(200, "application/json", "{\"ok\":true}"); });
}

void HomeService::processControlCommand(const String &jsonText)
{
    JsonDocument doc;
    const DeserializationError err = deserializeJson(doc, jsonText);
    if (err)
    {
        publishStatus(kStatusTopic, "error", String("invalid_control_json:") + err.c_str());
        return;
    }

    const String device = doc["device"] | "";

    if (device == "fan")
    {
        const String mode = doc["mode"] | "";
        if (mode == "off")
        {
            fan_.setMode(FanMode::Off);
        }
        else if (mode == "low")
        {
            fan_.setMode(FanMode::Low);
        }
        else if (mode == "medium")
        {
            fan_.setMode(FanMode::Medium);
        }
        else if (mode == "high")
        {
            fan_.setMode(FanMode::High);
        }

        if (!doc["speedPercent"].isNull())
        {
            fan_.setSpeedPercent(doc["speedPercent"].as<uint8_t>());
        }
        return;
    }

    if (device == "curtain")
    {
        if (!doc["angle"].isNull())
        {
            curtain_.setAngle(doc["angle"].as<uint8_t>());
        }
        if (!doc["preset"].isNull())
        {
            curtain_.setPresetLevel(doc["preset"].as<uint8_t>());
        }
        return;
    }

    if (device == "ir")
    {
        const String action = doc["action"] | "";

        if (action == "send")
        {
            const String protocol = doc["protocol"] | "NEC";
            const uint32_t address = doc["address"] | 0;
            const uint32_t command = doc["command"] | 0;
            const uint8_t repeats = doc["repeats"] | 0;
            const bool ok = ir_.sendProtocolCommand(protocol, address, command, repeats);
            publishStatus(kStatusTopic, ok ? "ir_command" : "error", ok ? "ir_protocol_sent" : "ir_protocol_send_failed");
            return;
        }

        if (action == "send_json")
        {
            const String commandJson = doc["commandJson"] | "";
            const bool ok = commandJson.length() > 0 && ir_.sendJsonCommand(commandJson);
            publishStatus(kStatusTopic, ok ? "ir_command" : "error", ok ? "ir_json_sent" : "ir_json_send_failed");
            return;
        }

        if (action.length() > 0)
        {
            String args = "{}";
            if (!doc["args"].isNull())
            {
                serializeJson(doc["args"], args);
            }
            const bool ok = ir_.sendActionCommand(action, args);
            publishStatus(kStatusTopic, ok ? "ir_command" : "error", ok ? "ir_action_sent" : "ir_action_send_failed");
            return;
        }

        publishStatus(kStatusTopic, "error", "ir_action_missing");
        return;
    }

    publishStatus(kStatusTopic, "error", "unknown_device");
}

void HomeService::handleSensorAndPublish()
{
    if (!sensors_.update())
    {
        return;
    }

    if (millis() - lastSensorPublishMs_ < 500)
    {
        return;
    }
    lastSensorPublishMs_ = millis();

    const SensorSnapshot &s = sensors_.snapshot();
    JsonDocument doc;
    doc["sensorType"] = "home_snapshot";
    doc["timestamp"] = s.timestamp;
    doc["temperatureC"] = s.temperatureC;
    doc["humidityPercent"] = s.humidityPercent;
    doc["lightPercent"] = s.lightPercent;
    doc["mq2Percent"] = s.mq2Percent;
    doc["smokeLevel"] = s.smokeLevel;
    doc["flameDetected"] = s.flameDetected;
    doc["error"] = s.hasError;
    if (s.hasError)
    {
        doc["errorMessage"] = s.errorMessage;
    }

    String payload;
    serializeJson(doc, payload);
    net_.mqttPublish(kSensorTopic, payload);
}

void HomeService::publishStatus(const char *topic, const String &type, const String &message) const
{
    JsonDocument doc;
    doc["type"] = type;
    doc["message"] = message;
    doc["timestamp"] = millis();

    String payload;
    serializeJson(doc, payload);
    const_cast<ConnectivityManager &>(net_).mqttPublish(topic, payload);
}

String HomeService::buildStatusJson(bool includeMeta) const
{
    const SensorSnapshot &s = sensors_.snapshot();

    JsonDocument doc;
    if (includeMeta)
    {
        doc["mode"] = net_.isCloudMode() ? "cloud" : "local_ap";
        doc["ip"] = net_.localIP().toString();
    }

    doc["temperatureC"] = s.temperatureC;
    doc["humidityPercent"] = s.humidityPercent;
    doc["lightPercent"] = s.lightPercent;
    doc["mq2Percent"] = s.mq2Percent;
    doc["smokeLevel"] = s.smokeLevel;
    doc["flameDetected"] = s.flameDetected;
    doc["fanSpeedPercent"] = fan_.speedPercent();
    doc["curtainAngle"] = curtain_.angle();
    doc["error"] = s.hasError;
    doc["errorMessage"] = s.errorMessage;

    String out;
    serializeJson(doc, out);
    return out;
}
