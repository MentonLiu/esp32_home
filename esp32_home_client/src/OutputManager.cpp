// 文件说明：esp32_home_client/src/OutputManager.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "OutputManager.h"

#include "ClientConfig.h"
#include "ClientLog.h"

namespace
{
    String formatTwoDigits(int value)
    {
        return value < 10 ? String("0") + value : String(value);
    }
} // namespace

OutputManager::OutputManager()
    : sensorOledWire_(0),
      statusOledWire_(1),
      sensorOled_(client_config::kOledWidth,
                  client_config::kOledHeight,
                  &sensorOledWire_,
                  client_config::kOledResetPin),
      statusOled_(client_config::kOledWidth,
                  client_config::kOledHeight,
                  &statusOledWire_,
                  client_config::kOledResetPin)
{
}

void OutputManager::begin()
{
    // 初始化输出外设并设置默认显示状态。
    const unsigned long startMs = millis();
    CL_INFO("REN", "output_begin", "phase=start");

    pinMode(client_config::kRgbRedPin, OUTPUT);
    pinMode(client_config::kRgbGreenPin, OUTPUT);
    pinMode(client_config::kRgbBluePin, OUTPUT);
    updateRgb("green");

    beginOleds();

    CL_INFOF("REN", "output_begin", "phase=done dur_ms=%lu", millis() - startMs);
}

void OutputManager::render(const ClientWiFiManager &wifiManager,
                           const ServerStatus &status,
                           const String &lastMessage,
                           bool serverReachable)
{
    // 即使帧被节流，也保持 LED 与时钟状态更新。
    updateRgb(status.smokeLevel);
    syncSystemTime(wifiManager);

    const unsigned long nowMs = millis();
    const unsigned long elapsedMs = nowMs - lastRenderMs_;
    if (elapsedMs < client_config::kDisplayRefreshIntervalMs)
    {
        if (client_log::allowPeriodic(lastRenderLogMs_, client_config::kDiagnosticsPeriodicLogMs))
        {
            CL_INFOF("REN", "frame_skip", "elapsed_ms=%lu target_ms=%lu", elapsedMs, client_config::kDisplayRefreshIntervalMs);
        }
        return;
    }
    lastRenderMs_ = nowMs;

    renderSensorOled(status);
    renderStatusOled(wifiManager, status, lastMessage, serverReachable);

    if (client_log::allowPeriodic(lastRenderLogMs_, client_config::kDiagnosticsPeriodicLogMs))
    {
        CL_INFOF("REN", "frame_done", "elapsed_ms=%lu", elapsedMs);
    }

    if (!firstFrameLogged_)
    {
        firstFrameLogged_ = true;
        CL_INFOF("REN", "first_frame", "since_boot_ms=%lu", client_log::sinceBootMs());
    }
}

void OutputManager::beginOleds()
{
    if (client_config::kEnableSensorOled)
    {
        const unsigned long sensorStartMs = millis();
        CL_INFO("REN", "sensor_oled_init", "phase=start");

        sensorOledWire_.begin(client_config::kSensorOledSda, client_config::kSensorOledScl);
        sensorOledReady_ = sensorOled_.begin(SSD1306_SWITCHCAPVCC, client_config::kSensorOledAddress);
        if (sensorOledReady_)
        {
            sensorOled_.clearDisplay();
            sensorOled_.setTextColor(SSD1306_WHITE);
            sensorOled_.setTextWrap(false);
            sensorOled_.display();
            CL_INFOF("REN", "sensor_oled_init", "phase=done dur_ms=%lu", millis() - sensorStartMs);
        }
        else
        {
            CL_WARN("REN", "sensor_oled_init", "phase=fail");
        }
    }

    if (client_config::kEnableStatusOled)
    {
        const unsigned long statusStartMs = millis();
        CL_INFO("REN", "status_oled_init", "phase=start");

        statusOledWire_.begin(client_config::kStatusOledSda, client_config::kStatusOledScl);
        statusOledReady_ = statusOled_.begin(SSD1306_SWITCHCAPVCC, client_config::kStatusOledAddress);
        if (statusOledReady_)
        {
            statusOled_.clearDisplay();
            statusOled_.setTextColor(SSD1306_WHITE);
            statusOled_.setTextWrap(false);
            statusOled_.display();
            CL_INFOF("REN", "status_oled_init", "phase=done dur_ms=%lu", millis() - statusStartMs);
        }
        else
        {
            CL_WARN("REN", "status_oled_init", "phase=fail");
        }
    }
}

void OutputManager::renderSensorOled(const ServerStatus &status)
{
    if (!sensorOledReady_)
    {
        return;
    }

    sensorOled_.clearDisplay();

    sensorOled_.setTextSize(1);
    sensorOled_.setCursor(0, 0);
    sensorOled_.print("SENSOR DATA");
    sensorOled_.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    sensorOled_.setTextSize(2);
    sensorOled_.setCursor(0, 16);
    sensorOled_.print(String(status.temperatureC, 1));
    sensorOled_.print("C");

    sensorOled_.setTextSize(1);
    sensorOled_.setCursor(0, 38);
    sensorOled_.print("HUM : ");
    sensorOled_.print(static_cast<int>(status.humidityPercent));
    sensorOled_.print("%");

    sensorOled_.setCursor(0, 48);
    sensorOled_.print("LIGHT: ");
    sensorOled_.print(status.lightPercent);
    sensorOled_.print("%");

    sensorOled_.setCursor(0, 58);
    sensorOled_.print("MQ2:");
    sensorOled_.print(status.mq2Percent);
    sensorOled_.print("% ");
    sensorOled_.print(status.flameDetected ? "FLAME" : "SAFE");

    sensorOled_.display();
}

void OutputManager::renderStatusOled(const ClientWiFiManager &wifiManager,
                                     const ServerStatus &status,
                                     const String &lastMessage,
                                     bool serverReachable)
{
    if (!statusOledReady_)
    {
        return;
    }

    statusOled_.clearDisplay();

    statusOled_.setTextSize(1);
    statusOled_.setCursor(0, 0);
    statusOled_.print("CTRL ");
    statusOled_.print(buildConnectionBadge(wifiManager));
    statusOled_.print(serverReachable ? " SRV" : " NOSRV");

    statusOled_.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    statusOled_.setTextSize(2);
    statusOled_.setCursor(0, 16);
    statusOled_.print(buildClockText());

    statusOled_.setTextSize(1);
    statusOled_.setCursor(0, 38);
    statusOled_.print(buildDateText());
    statusOled_.print(" ");
    statusOled_.print(buildWeekdayText());

    statusOled_.setCursor(0, 48);
    statusOled_.print("M:");
    statusOled_.print(fitLine(status.mode.length() > 0 ? status.mode : "offline", 8));
    statusOled_.print(" F:");
    statusOled_.print(fitLine(status.fanMode, 4));

    statusOled_.setCursor(0, 58);
    statusOled_.print("C:");
    statusOled_.print(status.curtainAngle);
    statusOled_.print(" ");
    statusOled_.print(fitLine(buildFooterText(status, lastMessage), 12));

    statusOled_.display();
}

void OutputManager::updateRgb(const String &smokeLevel)
{
    // 以红绿灯语义映射烟雾等级。
    bool red = false;
    bool green = false;
    bool blue = false;

    if (smokeLevel == "green")
    {
        green = true;
    }
    else if (smokeLevel == "blue")
    {
        blue = true;
    }
    else if (smokeLevel == "yellow")
    {
        red = true;
        green = true;
    }
    else
    {
        red = true;
    }

    digitalWrite(client_config::kRgbRedPin, red ? HIGH : LOW);
    digitalWrite(client_config::kRgbGreenPin, green ? HIGH : LOW);
    digitalWrite(client_config::kRgbBluePin, blue ? HIGH : LOW);
}

String OutputManager::buildClockText() const
{
    struct tm timeinfo;
    if (!getCurrentLocalTime(timeinfo))
    {
        return "--:--";
    }

    return formatTwoDigits(timeinfo.tm_hour) + ":" + formatTwoDigits(timeinfo.tm_min);
}

String OutputManager::buildDateText() const
{
    struct tm timeinfo;
    if (!getCurrentLocalTime(timeinfo))
    {
        return "--/--";
    }

    return formatTwoDigits(timeinfo.tm_mon + 1) + "/" + formatTwoDigits(timeinfo.tm_mday);
}

String OutputManager::buildWeekdayText() const
{
    struct tm timeinfo;
    if (!getCurrentLocalTime(timeinfo))
    {
        return "---";
    }

    static const char *const kWeekdays[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    return String(kWeekdays[timeinfo.tm_wday]);
}

String OutputManager::buildConnectionBadge(const ClientWiFiManager &wifiManager) const
{
    if (!wifiManager.isConnected())
    {
        return "OFF";
    }
    if (wifiManager.isConnectedToServerAp())
    {
        return "AP";
    }
    return "STA";
}

String OutputManager::buildFooterText(const ServerStatus &status,
                                      const String &lastMessage) const
{
    String footer = lastMessage;
    if (footer.length() == 0)
    {
        footer = status.flameDetected ? "flame" : buildSmokeLabel(status.smokeLevel);
    }
    return fitLine(footer, 24);
}

String OutputManager::buildSmokeLabel(const String &smokeLevel) const
{
    if (smokeLevel == "green")
    {
        return "safe";
    }
    if (smokeLevel == "blue")
    {
        return "cool";
    }
    if (smokeLevel == "yellow")
    {
        return "warn";
    }
    return "alarm";
}

String OutputManager::fitLine(const String &text, size_t maxLength) const
{
    if (text.length() <= maxLength)
    {
        return text;
    }
    return text.substring(0, maxLength);
}

void OutputManager::syncSystemTime(const ClientWiFiManager &wifiManager)
{
    if (!wifiManager.isConnected())
    {
        return;
    }

    if (!timeConfigApplied_)
    {
        configTzTime(client_config::kTimeZone,
                     client_config::kNtpServerPrimary,
                     client_config::kNtpServerSecondary);
        timeConfigApplied_ = true;
        lastTimeSyncAttemptMs_ = millis();
        CL_INFOF("TIME", "config_applied", "tz=%s ntp1=%s ntp2=%s",
                 client_config::kTimeZone,
                 client_config::kNtpServerPrimary,
                 client_config::kNtpServerSecondary);
        return;
    }

    if (timeSynced_)
    {
        return;
    }

    const unsigned long nowMs = millis();
    if (nowMs - lastTimeSyncAttemptMs_ < client_config::kTimeSyncRetryIntervalMs)
    {
        return;
    }
    lastTimeSyncAttemptMs_ = nowMs;

    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 100))
    {
        timeSynced_ = true;
        CL_INFOF("TIME", "sync_ok", "date=%04d-%02d-%02d time=%02d:%02d:%02d",
                 timeinfo.tm_year + 1900,
                 timeinfo.tm_mon + 1,
                 timeinfo.tm_mday,
                 timeinfo.tm_hour,
                 timeinfo.tm_min,
                 timeinfo.tm_sec);
    }
    else
    {
        CL_WARN("TIME", "sync_wait", "state=ntp_pending");
    }
}

bool OutputManager::getCurrentLocalTime(struct tm &timeinfo) const
{
    if (!getLocalTime(&timeinfo, 10))
    {
        return false;
    }

    return timeinfo.tm_year >= (2024 - 1900);
}
