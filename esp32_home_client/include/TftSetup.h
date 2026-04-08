#ifndef TFT_SETUP_H
#define TFT_SETUP_H

// ST7789 240x320 SPI 屏的 TFT_eSPI 用户配置。
#define ST7789_DRIVER

#define TFT_WIDTH 240
#define TFT_HEIGHT 320

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_MISO -1
#define TFT_CS 27
#define TFT_DC 16
#define TFT_RST 17

// 为 ESP32 稳定刷新调优的 SPI 频率。
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 16000000

// 启用所需的内置字体包。
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#endif
