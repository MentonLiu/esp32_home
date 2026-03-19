#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

static const char *AP_SSID = "ESP32_AP_TEST";
static const char *AP_PASS = "12345678";

WebServer server(80);
unsigned long lastPrintMs = 0;

void handleRoot()
{
  String html;
  html += "<html><head><meta charset='UTF-8'><title>ESP32 AP Test</title></head><body>";
  html += "<h1>ESP32 AP is running</h1>";
  html += "<p>SSID: ";
  html += AP_SSID;
  html += "</p><p>Connected devices: ";
  html += String(WiFi.softAPgetStationNum());
  html += "</p></body></html>";
  server.send(200, "text/html", html);
}

void setup()
{
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("Starting ESP32 SoftAP test...");

  bool started = WiFi.softAP(AP_SSID, AP_PASS);
  if (!started)
  {
    Serial.println("SoftAP start failed.");
    return;
  }

  IPAddress apIP = WiFi.softAPIP();

  Serial.print("AP started. SSID: ");
  Serial.println(AP_SSID);
  Serial.print("Password: ");
  Serial.println(AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(apIP);
  Serial.println("Connect phone to AP, then open browser: http://192.168.4.1");

  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started.");
}

void loop()
{
  server.handleClient();

  if (millis() - lastPrintMs >= 2000)
  {
    lastPrintMs = millis();
    Serial.print("Connected devices: ");
    Serial.println(WiFi.softAPgetStationNum());
  }
}