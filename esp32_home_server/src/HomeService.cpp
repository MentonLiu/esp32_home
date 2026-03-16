#include "HomeService.h"

#include <ArduinoJson.h>
#include <DHT.h>
#include <time.h>

#include "pins.h"

namespace
{
    const char *kStaSsid = "home-WiFi";
    const char *kStaPassword = "";
    const char *kApSsid = "esp32-server";
    const char *kApPassword = "lbl450981";

    const char *kMqttHost = "example.mqtt.server";
    const uint16_t kMqttPort = 1883;
    const char *kMqttClientId = "esp32-home-server";

    const char *kSensorTopic = "esp32/home/sensors";
    const char *kStatusTopic = "esp32/home/status";
    const char *kAlarmTopic = "esp32/home/alarm";
} // namespace

HomeService::HomeService()
    : sensors_(pins::DHT_DATA, DHT22, pins::LDR_ANALOG, pins::MQ2_ANALOG, pins::FLAME_ANALOG),
      fan_(pins::RELAY_FAN),
      curtain_(pins::SERVO_1, pins::SERVO_2),
      buzzer_(pins::BUZZER),
      ir_(pins::IR_RX, pins::IR_TX),
      net_(kStaSsid, kStaPassword, kApSsid, kApPassword)
{
    const int hh = (__TIME__[0] - '0') * 10 + (__TIME__[1] - '0');
    const int mm = (__TIME__[3] - '0') * 10 + (__TIME__[4] - '0');
    const int ss = (__TIME__[6] - '0') * 10 + (__TIME__[7] - '0');
    bootBaseSeconds_ = static_cast<uint32_t>(hh * 3600 + mm * 60 + ss);
}

void HomeService::begin()
{
    Serial.begin(115200);

    sensors_.begin();
    fan_.begin();
    curtain_.begin();
    buzzer_.begin();
    ir_.begin();

    net_.begin();
    net_.configureCloudMqtt(kMqttHost, kMqttPort, kMqttClientId);
    net_.setMqttHandler([this](char *topic, uint8_t *payload, unsigned int length)
                        {
    String body;
    body.reserve(length);
    for (unsigned int i = 0; i < length; ++i) {
      body += static_cast<char>(payload[i]);
    }
    processControlCommand(body);
    publishStatus(kStatusTopic, "command", String("received topic=") + topic); });

    setupWebRoutes();
    net_.webServer().begin();

    if (net_.isInternetConnected())
    {
        configTime(8 * 3600, 0, "pool.ntp.org", "ntp.aliyun.com");
        ntpConfigured_ = true;
    }

    Serial.println("Home service started");
}

void HomeService::loop()
{
    net_.loop();
    handleSensorAndPublish();
    handleAutomation();
    handleAlerts();

    const IRDecodedSignal signal = ir_.receive();
    if (signal.available)
    {
        DynamicJsonDocument doc(256);
        doc["type"] = "ir_receive";
        doc["protocol"] = signal.protocol;
        doc["address"] = signal.address;
        doc["command"] = signal.command;
        doc["raw"] = signal.rawData;
        String payload;
        serializeJson(doc, payload);
        publishStatus(kStatusTopic, "ir", payload);
    }
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
    DynamicJsonDocument doc(512);
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

        if (doc.containsKey("speedPercent"))
        {
            fan_.setSpeedPercent(doc["speedPercent"].as<uint8_t>());
        }
        return;
    }

    if (device == "curtain")
    {
        if (doc.containsKey("angle"))
        {
            curtain_.setAngle(doc["angle"].as<uint8_t>());
        }
        if (doc.containsKey("preset"))
        {
            curtain_.setPresetLevel(doc["preset"].as<uint8_t>());
        }
        return;
    }

    if (device == "ir")
    {
        const String action = doc["action"] | "";
        if (action == "send_nec")
        {
            const uint16_t address = doc["address"] | 0;
            const uint8_t command = doc["command"] | 0;
            const uint8_t repeats = doc["repeats"] | 0;
            ir_.sendNEC(address, command, repeats);
        }
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
    DynamicJsonDocument doc(512);
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

void HomeService::handleAutomation()
{
    if (millis() - lastAutomationMs_ < 1000)
    {
        return;
    }
    lastAutomationMs_ = millis();

    if (net_.isInternetConnected() && !ntpConfigured_)
    {
        configTime(8 * 3600, 0, "pool.ntp.org", "ntp.aliyun.com");
        ntpConfigured_ = true;
    }

    const int hour = currentHour();
    const int minute = currentMinute();
    const uint32_t dayIndex = currentDayIndex();

    if (hour == 7 && minute == 0 && lastOpenDay_ != dayIndex)
    {
        curtain_.setPresetLevel(4);
        lastOpenDay_ = dayIndex;
        publishStatus(kStatusTopic, "automation", "curtain_open_07_00");
    }

    if (hour == 22 && minute == 0 && lastCloseDay_ != dayIndex)
    {
        curtain_.setPresetLevel(0);
        lastCloseDay_ = dayIndex;
        publishStatus(kStatusTopic, "automation", "curtain_close_22_00");
    }
}

void HomeService::handleAlerts()
{
    const SensorSnapshot &s = sensors_.snapshot();

    if (s.mq2Percent >= 75)
    {
        fan_.setMode(FanMode::High);
    }

    if (s.mq2Percent >= 90 && millis() - lastHighSmokeBeepMs_ >= 1000)
    {
        lastHighSmokeBeepMs_ = millis();
        buzzer_.beep(2600, 80);
    }

    if (s.flameDetected)
    {
        if (flameDetectedSinceMs_ == 0)
        {
            flameDetectedSinceMs_ = millis();
            fireAlarmReported_ = false;
        }

        const unsigned long flameDuration = millis() - flameDetectedSinceMs_;
        if (flameDuration >= 45000 && millis() - lastFlamePatternMs_ >= 2500)
        {
            lastFlamePatternMs_ = millis();
            buzzer_.patternShortShortLong();
        }

        if (flameDuration >= 300000 && !fireAlarmReported_ && net_.isCloudMode())
        {
            fireAlarmReported_ = true;
            publishStatus(kAlarmTopic, "fire", "flame_over_5min_fire_service_alert");
        }
    }
    else
    {
        flameDetectedSinceMs_ = 0;
        fireAlarmReported_ = false;
    }
}

void HomeService::publishStatus(const char *topic, const String &type, const String &message) const
{
    DynamicJsonDocument doc(512);
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

    DynamicJsonDocument doc(768);
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

int HomeService::currentHour() const
{
    if (ntpConfigured_)
    {
        time_t now = time(nullptr);
        if (now > 100000)
        {
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            return timeinfo.tm_hour;
        }
    }

    const uint32_t seconds = bootBaseSeconds_ + millis() / 1000;
    return (seconds / 3600) % 24;
}

int HomeService::currentMinute() const
{
    if (ntpConfigured_)
    {
        time_t now = time(nullptr);
        if (now > 100000)
        {
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            return timeinfo.tm_min;
        }
    }

    const uint32_t seconds = bootBaseSeconds_ + millis() / 1000;
    return (seconds / 60) % 60;
}

uint32_t HomeService::currentDayIndex() const
{
    if (ntpConfigured_)
    {
        time_t now = time(nullptr);
        if (now > 100000)
        {
            return static_cast<uint32_t>(now / 86400);
        }
    }

    const uint32_t seconds = bootBaseSeconds_ + millis() / 1000;
    return seconds / 86400;
}
