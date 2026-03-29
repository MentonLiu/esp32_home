// 文件说明：esp32_home_client/src/OutputManager.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "OutputManager.h"

#include "ClientConfig.h"
#include "ClientLog.h"

namespace
{
    constexpr uint16_t kBgTop = TFT_BLACK;
    constexpr uint16_t kBgBottom = 0x18E7;
    constexpr uint16_t kCardFill = 0x4208;
    constexpr uint16_t kCardBorder = 0x94B2;
    constexpr uint16_t kPanelFill = 0x2969;
    constexpr uint16_t kTextPrimary = TFT_WHITE;
    constexpr uint16_t kTextMuted = 0xC638;
    constexpr uint16_t kDotOff = 0x630C;

    const char *monthFromDateToken(const String &token)
    {
        if (token == "Jan")
            return "Jan";
        if (token == "Feb")
            return "Feb";
        if (token == "Mar")
            return "Mar";
        if (token == "Apr")
            return "Apr";
        if (token == "May")
            return "May";
        if (token == "Jun")
            return "Jun";
        if (token == "Jul")
            return "Jul";
        if (token == "Aug")
            return "Aug";
        if (token == "Sep")
            return "Sep";
        if (token == "Oct")
            return "Oct";
        if (token == "Nov")
            return "Nov";
        return "Dec";
    }

    int monthIndexFromToken(const String &token)
    {
        if (token == "Jan")
            return 1;
        if (token == "Feb")
            return 2;
        if (token == "Mar")
            return 3;
        if (token == "Apr")
            return 4;
        if (token == "May")
            return 5;
        if (token == "Jun")
            return 6;
        if (token == "Jul")
            return 7;
        if (token == "Aug")
            return 8;
        if (token == "Sep")
            return 9;
        if (token == "Oct")
            return 10;
        if (token == "Nov")
            return 11;
        return 12;
    }

    int weekdayIndex(int year, int month, int day)
    {
        if (month < 3)
        {
            month += 12;
            year -= 1;
        }

        const int k = year % 100;
        const int j = year / 100;
        const int h = (day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
        return (h + 5) % 7;
    }

    const char *weekdayName(int index)
    {
        static const char *const kWeekdays[] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
        if (index < 0 || index > 6)
        {
            return "DAY";
        }
        return kWeekdays[index];
    }

    String formatTwoDigits(unsigned long value)
    {
        return value < 10 ? String("0") + value : String(value);
    }
} // namespace

OutputManager::OutputManager()
    : lcd_(client_config::kLcdAddress, client_config::kLcdColumns, client_config::kLcdRows),
      tft_()
{
}

void OutputManager::begin()
{
    const unsigned long startMs = millis();
    CL_INFO("REN", "output_begin", "phase=start");

    pinMode(client_config::kRgbRedPin, OUTPUT);
    pinMode(client_config::kRgbGreenPin, OUTPUT);
    pinMode(client_config::kRgbBluePin, OUTPUT);
    updateRgb("green");

    if (client_config::kEnableLcd1602)
    {
        const unsigned long lcdStartMs = millis();
        CL_INFO("REN", "lcd_init", "phase=start");
        Wire.begin(client_config::kLcdSda, client_config::kLcdScl);
        lcd_.init();
        lcd_.backlight();
        lcd_.clear();
        lcdReady_ = true;
        CL_INFOF("REN", "lcd_init", "phase=done dur_ms=%lu", millis() - lcdStartMs);
    }

    if (client_config::kEnableTft)
    {
        beginTft();
    }

    CL_INFOF("REN", "output_begin", "phase=done dur_ms=%lu", millis() - startMs);
}

void OutputManager::render(const ClientWiFiManager &wifiManager,
                           const ServerStatus &status,
                           const String &lastMessage,
                           bool serverReachable)
{
    updateRgb(status.smokeLevel);

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

    renderTftHtmlPage(wifiManager, status, lastMessage, serverReachable);
    renderLcd(wifiManager, status);

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

void OutputManager::beginTft()
{
    const unsigned long startMs = millis();
    CL_INFO("REN", "tft_init", "phase=start");

    pinMode(client_config::kTftBacklight, OUTPUT);
    digitalWrite(client_config::kTftBacklight, HIGH);

    tft_.init();
    tft_.setRotation(0);
    tft_.fillScreen(TFT_BLACK);
    tftStaticPainted_ = false;
    tftReady_ = true;
    firstFrameLogged_ = false;
    CL_INFOF("REN", "tft_init", "phase=done dur_ms=%lu", millis() - startMs);
}

void OutputManager::drawTftStaticLayout()
{
    const unsigned long startMs = millis();
    CL_INFO("REN", "tft_static", "phase=start");

    for (int y = 0; y < client_config::kTftHeight; ++y)
    {
        const uint8_t mix = static_cast<uint8_t>((y * 255) / client_config::kTftHeight);
        tft_.drawFastHLine(0, y, client_config::kTftWidth, tft_.alphaBlend(mix, kBgBottom, kBgTop));
    }

    tft_.fillRoundRect(10, 10, 220, 300, 15, kCardFill);
    tft_.drawRoundRect(10, 10, 220, 300, 15, kCardBorder);

    tft_.setTextColor(kTextMuted, kCardFill);
    tft_.setTextFont(2);
    tft_.drawString("ESP32-HOME", 25, 27);

    tft_.setTextColor(kTextPrimary, kCardFill);
    tft_.setTextFont(2);
    tft_.drawCentreString("MADE IN 600", 120, 292, 2);

    tft_.fillRoundRect(24, 58, 130, 28, 8, kPanelFill);
    tft_.fillRoundRect(24, 186, 166, 50, 10, kPanelFill);
    tft_.setTextColor(kTextMuted, kPanelFill);
    tft_.setTextFont(2);
    tft_.drawString("TEMP / HUM / FAN", 32, 196);
    tft_.fillRoundRect(24, 242, 166, 28, 8, kPanelFill);

    CL_INFOF("REN", "tft_static", "phase=done dur_ms=%lu", millis() - startMs);
}

void OutputManager::renderTftHtmlPage(const ClientWiFiManager &wifiManager,
                                      const ServerStatus &status,
                                      const String &lastMessage,
                                      bool serverReachable)
{
    if (!tftReady_)
    {
        return;
    }

    if (!tftStaticPainted_)
    {
        drawTftStaticLayout();
        tftStaticPainted_ = true;
    }

    tft_.fillRect(174, 23, 48, 22, kCardFill);
    tft_.setTextColor(kTextPrimary, kCardFill);
    tft_.setTextFont(4);
    tft_.drawString(buildConnectionBadge(wifiManager), 186, 27);

    tft_.fillRect(25, 104, 140, 54, kCardFill);
    tft_.setTextFont(7);
    tft_.drawString(buildClockText(), 25, 108);

    tft_.fillRect(24, 256, 170, 18, kCardFill);
    tft_.setTextFont(2);
    tft_.setTextColor(kTextMuted, kCardFill);
    tft_.drawString(buildDateText(), 28, 257);
    tft_.drawString(buildWeekdayText(), 120, 257);

    const uint16_t wifiDot = wifiManager.isConnected() ? TFT_CYAN : kDotOff;
    const uint16_t serverDot = serverReachable ? TFT_GREEN : kDotOff;
    const uint16_t smokeDot = smokeColor565(status.smokeLevel);
    tft_.fillCircle(207, 205, 8, smokeDot);
    tft_.fillCircle(207, 234, 8, serverDot);
    tft_.fillCircle(207, 263, 8, wifiDot);

    tft_.fillRoundRect(24, 58, 130, 28, 8, kPanelFill);
    tft_.setTextColor(kTextPrimary, kPanelFill);
    tft_.setTextFont(2);
    tft_.drawString(status.mode.length() > 0 ? status.mode : "offline", 32, 66);

    tft_.fillRoundRect(24, 186, 166, 50, 10, kPanelFill);
    tft_.setTextColor(kTextMuted, kPanelFill);
    tft_.drawString("TEMP / HUM / FAN", 32, 196);
    tft_.setTextColor(kTextPrimary, kPanelFill);
    const String liveInfo = buildLiveInfo(status);
    tft_.drawString(liveInfo, 32, 214);

    tft_.fillRoundRect(24, 242, 166, 28, 8, kPanelFill);
    tft_.setTextColor(kTextPrimary, kPanelFill);
    const String footer = buildFooterText(status, lastMessage);
    tft_.drawString(footer, 32, 250);
}

void OutputManager::updateRgb(const String &smokeLevel)
{
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

void OutputManager::renderLcd(const ClientWiFiManager &wifiManager,
                              const ServerStatus &status)
{
    if (!lcdReady_)
    {
        return;
    }

    String line1 = wifiManager.isConnected() ? wifiManager.currentSsid() : "wifi offline";
    if (line1.length() == 0)
    {
        line1 = "wifi waiting";
    }

    String line2 = String("C") + status.curtainAngle +
                   " F" + status.fanMode +
                   " " + status.smokeLevel;

    lcd_.setCursor(0, 0);
    lcd_.print(fit16(line1));
    lcd_.setCursor(0, 1);
    lcd_.print(fit16(line2));
}

String OutputManager::fit16(const String &text) const
{
    String out = text;
    if (out.length() > 16)
    {
        out = out.substring(0, 16);
    }
    while (out.length() < 16)
    {
        out += ' ';
    }
    return out;
}

String OutputManager::buildClockText() const
{
    const unsigned long totalMinutes = millis() / 60000UL;
    const unsigned long hours = (12UL + (totalMinutes / 60UL)) % 24UL;
    const unsigned long minutes = totalMinutes % 60UL;
    return formatTwoDigits(hours) + ":" + formatTwoDigits(minutes);
}

String OutputManager::buildDateText() const
{
    const String buildDate = __DATE__;
    const String monthToken = buildDate.substring(0, 3);
    const String dayToken = buildDate.substring(4, 6);
    const String day = String(dayToken.toInt());
    return String(monthFromDateToken(monthToken)) + " " + day;
}

String OutputManager::buildWeekdayText() const
{
    const String buildDate = __DATE__;
    const String monthToken = buildDate.substring(0, 3);
    const int month = monthIndexFromToken(monthToken);
    const int day = buildDate.substring(4, 6).toInt();
    const int year = buildDate.substring(7).toInt();
    return String(weekdayName(weekdayIndex(year, month, day)));
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

String OutputManager::buildLiveInfo(const ServerStatus &status) const
{
    return String(status.temperatureC, 1) + "C  " +
           String(static_cast<int>(status.humidityPercent)) + "%  " +
           status.fanMode;
}

String OutputManager::buildFooterText(const ServerStatus &status,
                                      const String &lastMessage) const
{
    String footer = lastMessage;
    if (footer.length() == 0)
    {
        footer = status.flameDetected ? "flame_detected" : "screen_ready";
    }
    if (footer.length() > 24)
    {
        footer = footer.substring(0, 24);
    }
    return footer;
}

uint16_t OutputManager::smokeColor565(const String &smokeLevel) const
{
    if (smokeLevel == "green")
    {
        return TFT_GREEN;
    }
    if (smokeLevel == "blue")
    {
        return TFT_CYAN;
    }
    if (smokeLevel == "yellow")
    {
        return TFT_ORANGE;
    }
    return TFT_RED;
}
