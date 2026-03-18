/**
 * @file HomeServiceControl.cpp
 * @brief HomeService 的 Web、控制与状态发布实现
 */

#include "HomeService.h"

#include <ArduinoJson.h>

namespace
{
    const char *kSensorTopic = "esp32/home/sensors";
    const char *kStatusTopic = "esp32/home/status";
}

void HomeService::setupWebRoutes()
{
    net_.webServer().on("/", HTTP_GET, [this]()
                        {
    const char html[] = R"HTML(
<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>ESP32 Home Console</title>
  <style>
    :root { --bg:#f4f7f1; --panel:#ffffff; --text:#1f2a1f; --accent:#2f7d32; --warn:#cc7a00; --danger:#c62828; }
    body { margin:0; font-family:"Trebuchet MS", "Segoe UI", sans-serif; background:linear-gradient(160deg,#e6efe0,#fdf8ec); color:var(--text); }
    .wrap { max-width:980px; margin:0 auto; padding:20px; }
    .card { background:var(--panel); border-radius:16px; padding:16px; box-shadow:0 8px 24px rgba(0,0,0,0.08); margin-bottom:14px; }
    h1 { margin:0 0 12px; font-size:1.6rem; }
    .grid { display:grid; grid-template-columns:repeat(auto-fit,minmax(180px,1fr)); gap:10px; }
    .k { font-size:0.86rem; opacity:0.7; }
    .v { font-size:1.2rem; font-weight:700; }
    button { border:0; border-radius:10px; padding:10px 14px; margin:4px; background:var(--accent); color:#fff; font-weight:700; }
    .warn { background:var(--warn); }
    .danger { background:var(--danger); }
    input[type=range] { width:100%; }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card"><h1>ESP32 Home Console</h1><div id="net"></div></div>
    <div class="card grid" id="sensorGrid"></div>
    <div class="card">
      <div><b>Curtain</b></div>
      <input id="curtainRange" type="range" min="0" max="180" value="0" />
      <div>
        <button onclick="sendCmd({device:'curtain',preset:0})">Close</button>
        <button onclick="sendCmd({device:'curtain',preset:2})">Half</button>
        <button onclick="sendCmd({device:'curtain',preset:4})">Open</button>
      </div>
    </div>
    <div class="card">
      <div><b>Fan</b></div>
      <div>
        <button onclick="sendCmd({device:'fan',mode:'off'})" class="danger">Off</button>
        <button onclick="sendCmd({device:'fan',mode:'low'})">Low</button>
        <button onclick="sendCmd({device:'fan',mode:'medium'})">Medium</button>
        <button onclick="sendCmd({device:'fan',mode:'high'})" class="warn">High</button>
      </div>
    </div>
  </div>
<script>
async function fetchStatus() {
  const res = await fetch('/api/status');
  const data = await res.json();
  document.getElementById('net').textContent = `mode=${data.mode} ip=${data.ip}`;
  document.getElementById('sensorGrid').innerHTML = `
    <div><div class='k'>temperature</div><div class='v'>${data.temperatureC} C</div></div>
    <div><div class='k'>humidity</div><div class='v'>${data.humidityPercent} %</div></div>
    <div><div class='k'>light</div><div class='v'>${data.lightPercent} %</div></div>
    <div><div class='k'>smoke</div><div class='v'>${data.smokeLevel} (${data.mq2Percent}%)</div></div>
    <div><div class='k'>flame</div><div class='v'>${data.flameDetected ? 'YES':'NO'}</div></div>
    <div><div class='k'>fan</div><div class='v'>${data.fanSpeedPercent}%</div></div>
    <div><div class='k'>curtain</div><div class='v'>${data.curtainAngle} deg</div></div>
  `;
  document.getElementById('curtainRange').value = data.curtainAngle;
}

async function sendCmd(body) {
  await fetch('/api/control', { method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(body) });
}

document.getElementById('curtainRange').addEventListener('input', (e) => {
  sendCmd({device:'curtain', angle:Number(e.target.value)});
});

setInterval(fetchStatus, 500);
fetchStatus();
</script>
</body>
</html>
)HTML";
    net_.webServer().send(200, "text/html", html); });

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
