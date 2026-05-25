#include "ST7789.h"
#include <string.h>
#include "fonts.h"

#define LCD_W 240
#define LCD_H 240

ST7789::ST7789(uint8_t cs, uint8_t dc, uint8_t rst, uint8_t blk)
    : _cs(cs), _dc(dc), _rst(rst), _blk(blk) {}

// ===== LOW LEVEL =====
void ST7789::writeCommand(uint8_t cmd)
{
    digitalWrite(_dc, LOW);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;

    spi_device_transmit(spi, &t);
}

void ST7789::writeData(uint8_t data)
{
    digitalWrite(_dc, HIGH);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &data;

    spi_device_transmit(spi, &t);
}

// ===== INIT =====
void ST7789::begin()
{
    pinMode(_dc, OUTPUT);
    pinMode(_rst, OUTPUT);

    if (_blk != 255)
    {
        pinMode(_blk, OUTPUT);
        digitalWrite(_blk, HIGH);
    }

    // ===== FIX: không dùng designated initializer =====
    spi_bus_config_t buscfg;
    memset(&buscfg, 0, sizeof(buscfg));
    buscfg.mosi_io_num = 23;
    buscfg.miso_io_num = -1;
    buscfg.sclk_io_num = 18;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;

    spi_bus_initialize(VSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg;
    memset(&devcfg, 0, sizeof(devcfg));
    devcfg.clock_speed_hz = 40000000;
    devcfg.mode = 0;
    devcfg.spics_io_num = _cs;
    devcfg.queue_size = 7;

    spi_bus_add_device(VSPI_HOST, &devcfg, &spi);

    // reset LCD
    digitalWrite(_rst, LOW);
    delay(50);
    digitalWrite(_rst, HIGH);
    delay(50);

    writeCommand(0x01);
    delay(150);
    writeCommand(0x11);
    delay(150);

    writeCommand(0x36);
    writeData(0x00);
    writeCommand(0x3A);
    writeData(0x05);
    writeCommand(0x21);

    writeCommand(0x29);
    delay(50);
}

// ===== ADDRESS WINDOW =====
void ST7789::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    writeCommand(0x2A);

    uint8_t data_col[] = {
        (uint8_t)(x0 >> 8), (uint8_t)(x0 & 0xFF),
        (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF)};

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    digitalWrite(_dc, HIGH);
    t.length = 32;
    t.tx_buffer = data_col;
    spi_device_transmit(spi, &t);

    writeCommand(0x2B);

    uint8_t data_row[] = {
        (uint8_t)(y0 >> 8), (uint8_t)(y0 & 0xFF),
        (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF)};

    memset(&t, 0, sizeof(t));
    digitalWrite(_dc, HIGH);
    t.length = 32;
    t.tx_buffer = data_row;
    spi_device_transmit(spi, &t);

    writeCommand(0x2C);
}

// ===== DRAW =====
void ST7789::drawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= LCD_W || y >= LCD_H)
        return;

    setAddrWindow(x, y, x, y);

    uint16_t c = __builtin_bswap16(color);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    digitalWrite(_dc, HIGH);
    t.length = 16;
    t.tx_buffer = &c;

    spi_device_transmit(spi, &t);
}

// ===== FAST FILL (DMA) =====
void ST7789::fillScreen(uint16_t color)
{
    setAddrWindow(0, 0, LCD_W - 1, LCD_H - 1);

    digitalWrite(_dc, HIGH);

    const int pixels = LCD_W * LCD_H;
    const int chunk = 1024;

    static uint16_t buffer[chunk];

    uint16_t c = __builtin_bswap16(color);
    for (int i = 0; i < chunk; i++)
    {
        buffer[i] = c;
    }

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    for (int i = 0; i < pixels; i += chunk)
    {
        int size = (pixels - i > chunk) ? chunk : (pixels - i);

        t.length = size * 16;
        t.tx_buffer = buffer;

        spi_device_transmit(spi, &t);
    }
}

void ST7789::drawChar(int x, int y, char c, uint16_t color, uint16_t bg)
{
    // Kiểm tra xem ký tự có nằm trong bảng font 128 không
    if ((uint8_t)c > 127)
        return;

    // Lấy bitmap trực tiếp từ chỉ số ASCII
    const uint8_t *bitmap = font5x7[(uint8_t)c];

    for (int i = 0; i < 5; i++)
    {
        uint8_t line = bitmap[i];
        for (int j = 0; j < 7; j++)
        {
            if (line & (1 << j))
            {
                drawPixel(x + i, y + j, color);
            }
            else
            {
                drawPixel(x + i, y + j, bg);
            }
        }
    }
}

void ST7789::drawString(int x, int y, const char *str, uint16_t color, uint16_t bg)
{
    while (*str)
    {
        drawChar(x, y, *str, color, bg);
        x += 6; // spacing
        str++;
    }
}

void ST7789::drawBitmap(int x, int y, const uint16_t *img, int w, int h, uint16_t color)
{
    // 1. Kiểm tra biên vẽ: chỉ vẽ nếu toàn bộ hoặc một phần cây nằm trong màn hình
    if (x + w <= 0 || x >= 240 || y + h <= 0 || y >= 240)
        return;

    // 2. Tính toán vùng cửa sổ vẽ chính xác (Clipped area)
    int draw_x = x;
    int draw_y = y;
    int draw_w = w;
    int draw_h = h;
    const uint16_t *bitmap_ptr = img;

    if (x < 0)
    {
        draw_w += x;        // Giảm chiều rộng vẽ
        bitmap_ptr += (-x); // Bỏ qua pixel bitmap bị cắt bên trái
        draw_x = 0;         // Bắt đầu vẽ từ rìa trái màn hình
    }
    if (x + w > 240)
        draw_w = 240 - x;

    if (y < 0)
    {
        draw_h += y; // Giảm chiều cao vẽ
        draw_y = 0;  // Bắt đầu vẽ từ rìa trên màn hình
    }
    if (y + h > 240)
        draw_h = 240 - y;

    // 3. Thiết lập cửa sổ vẽ CHÍNH XÁC
    setAddrWindow(draw_x, draw_y, draw_x + draw_w - 1, draw_y + draw_h - 1);
    digitalWrite(_dc, HIGH);

    // 4. Vẽ dữ liệu bitmap
    uint16_t color_sw = __builtin_bswap16(color);
    uint16_t black_sw = 0x0000;
    int total = draw_w * draw_h;
    const int CHUNK = 1024;
    static uint16_t buffer[CHUNK];
    int index = 0;

    // Phải quét bitmap gốc đúng cách để lấy dữ liệu được crop
    for (int current_h = 0; current_h < draw_h; current_h++)
    {
        const uint16_t *line_ptr = bitmap_ptr + (current_h * w); // Lấy con trỏ đầu mỗi hàng
        for (int current_w = 0; current_w < draw_w; current_w++)
        {
            buffer[index++] = (line_ptr[current_w] > 0) ? color_sw : black_sw;
            if (index >= CHUNK)
            {
                // Gửi buffer
                spi_transaction_t t = {};
                t.length = index * 16;
                t.tx_buffer = buffer;
                spi_device_transmit(spi, &t);
                index = 0;
            }
        }
    }

    if (index > 0)
    {
        spi_transaction_t t = {};
        t.length = index * 16;
        t.tx_buffer = buffer;
        spi_device_transmit(spi, &t);
    }
}

void ST7789::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if ((x >= LCD_W) || (y >= LCD_H))
        return;
    if ((x + w - 1) >= LCD_W)
        w = LCD_W - x;
    if ((y + h - 1) >= LCD_H)
        h = LCD_H - y;
    if (w <= 0 || h <= 0)
        return;

    setAddrWindow(x, y, x + w - 1, y + h - 1);
    digitalWrite(_dc, HIGH);
    uint16_t c = __builtin_bswap16(color);

    const int CHUNK = 1024;
    static uint16_t buffer[CHUNK];
    for (int i = 0; i < CHUNK; i++)
        buffer[i] = c;

    int total = w * h;
    while (total > 0)
    {
        int size = (total > CHUNK) ? CHUNK : total;
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = size * 16;
        t.tx_buffer = buffer;
        spi_device_transmit(spi, &t);
        total -= size;
    }
}

void ST7789::drawLine(int x0, int y0, int x1, int y1, uint16_t color)
{
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;

    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;
    int e2;

    while (true)
    {
        drawPixel(x0, y0, color);

        if (x0 == x1 && y0 == y1)
            break;

        e2 = 2 * err;

        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void ST7789::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    drawLine(x, y, x + w - 1, y, color);                 // Cạnh trên
    drawLine(x, y + h - 1, x + w - 1, y + h - 1, color); // Cạnh dưới
    drawLine(x, y, x, y + h - 1, color);                 // Cạnh trái
    drawLine(x + w - 1, y, x + w - 1, y + h - 1, color); // Cạnh phải
}