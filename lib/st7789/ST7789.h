#ifndef ST7789_H
#define ST7789_H

#include <Arduino.h>
#include "driver/spi_master.h"

// RGB565 Color Definitions
#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE 0x001F
#define COLOR_YELLOW 0xFFE0
#define COLOR_MAROON 0x7800
#define COLOR_CYAN 0x07FF
#define COLOR_GRAY 0x7BEF

class ST7789
{
public:
    ST7789(uint8_t cs, uint8_t dc, uint8_t rst, uint8_t blk);
    void begin();
    void fillScreen(uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawPixel(uint16_t x, uint16_t y, uint16_t color);
    void drawBitmap(int x, int y, const uint16_t *img, int w, int h, uint16_t color);
    void drawString(int x, int y, const char *str, uint16_t color, uint16_t bg);
    void drawChar(int x, int y, char c, uint16_t color, uint16_t bg);
    void drawLine(int x0, int y0, int x1, int y1, uint16_t color);

    spi_device_handle_t spi;

private:
    uint8_t _cs, _dc, _rst, _blk;
    void writeCommand(uint8_t cmd);
    void writeData(uint8_t data);
    void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
};

#endif