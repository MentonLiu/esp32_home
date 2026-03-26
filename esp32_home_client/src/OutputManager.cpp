// 文件说明：esp32_home_client/src/OutputManager.cpp
// 该文件属于 ESP32 Home 项目，用于对应模块的声明或实现。

#include "OutputManager.h"

#include "ClientConfig.h"

OutputManager::OutputManager()
    : lcd_(client_config::kLcdAddress, client_config::kLcdColumns, client_config::kLcdRows),
      tft_()
{
}

void OutputManager::begin()
{
    pinMode(client_config::kRgbRedPin, OUTPUT);
    pinMode(client_config::kRgbGreenPin, OUTPUT);
    pinMode(client_config::kRgbBluePin, OUTPUT);
    updateRgb("green");

    if (client_config::kEnableLcd1602)
    {
        Wire.begin(client_config::kLcdSda, client_config::kLcdScl);
        lcd_.init();
        lcd_.backlight();
        lcd_.clear();
        lcdReady_ = true;
    }

    if (client_config::kEnableTft)
    {
        beginTft();
    }
}

void OutputManager::render(const ClientWiFiManager &wifiManager,
                           const ServerStatus &status,
                           const String &lastMessage,
                           bool serverReachable)
{
    updateRgb(status.smokeLevel);
    renderLvgl();

    if (millis() - lastRenderMs_ < client_config::kDisplayRefreshIntervalMs)
    {
        return;
    }
    lastRenderMs_ = millis();

    renderLcd(wifiManager, status);
    (void)lastMessage;
    (void)serverReachable;
}

void OutputManager::flushDisplay(lv_disp_drv_t *dispDriver, const lv_area_t *area, lv_color_t *colorMap)
{
    auto *instance = static_cast<OutputManager *>(dispDriver->user_data);
    if (instance == nullptr)
    {
        lv_disp_flush_ready(dispDriver);
        return;
    }

    const uint32_t width = static_cast<uint32_t>(area->x2 - area->x1 + 1);
    const uint32_t height = static_cast<uint32_t>(area->y2 - area->y1 + 1);

    instance->tft_.startWrite();
    instance->tft_.setAddrWindow(area->x1, area->y1, width, height);
    instance->tft_.pushColors(reinterpret_cast<uint16_t *>(colorMap), width * height, true);
    instance->tft_.endWrite();

    lv_disp_flush_ready(dispDriver);
}

void OutputManager::beginTft()
{
    pinMode(client_config::kTftBacklight, OUTPUT);
    digitalWrite(client_config::kTftBacklight, HIGH);

    tft_.init();
    tft_.setRotation(0);
    tft_.fillScreen(TFT_BLACK);

    lv_init();
    lv_disp_draw_buf_init(&lvDrawBuf_,
                          lvDrawBuffer_,
                          nullptr,
                          client_config::kTftWidth * 20);
    lv_disp_drv_init(&lvDispDrv_);
    lvDispDrv_.hor_res = client_config::kTftWidth;
    lvDispDrv_.ver_res = client_config::kTftHeight;
    lvDispDrv_.flush_cb = flushDisplay;
    lvDispDrv_.draw_buf = &lvDrawBuf_;
    lvDispDrv_.user_data = this;
    lv_disp_drv_register(&lvDispDrv_);

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
    lastLvTickMs_ = millis();
    tftReady_ = true;
}

void OutputManager::renderLvgl()
{
    if (!tftReady_)
    {
        return;
    }

    const unsigned long now = millis();
    const unsigned long elapsed = now - lastLvTickMs_;
    lastLvTickMs_ = now;
    lv_tick_inc(elapsed);
    lv_timer_handler();
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
