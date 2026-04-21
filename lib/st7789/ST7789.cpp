#include "ST7789.h"
#include <string.h>

#define LCD_W 240
#define LCD_H 240

static const uint8_t font5x7[128][5] = {
    {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, // 0-3
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0}, // 4-7
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0}, // 8-11
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0}, // 12-15
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0}, // 16-19
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0}, // 20-23
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0}, // 24-27
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0}, // 28-31

    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // 42 *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 /

    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 9

    {0x00, 0x67, 0x67, 0x00, 0x00}, // 58 :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 <
    {0x24, 0x24, 0x24, 0x24, 0x24}, // 61 =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 ?
    {0x32, 0x49, 0x59, 0x51, 0x3E}, // 64 @

    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 71 G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 L
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // 77 M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 V
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // 87 W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 Z

    {0x00, 0x7F, 0x41, 0x41, 0x00}, // 91 [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 92 "\"
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // 93 ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // 96 `

    {0x20, 0x54, 0x54, 0x54, 0x78}, // 97 a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 98 b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 99 c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 100 d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 102 f
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 103 g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 106 j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 107 k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 112 p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 113 q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 116 t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 z
    {0x08, 0x36, 0x41, 0x41, 0x00}, // 123 {
    {0x00, 0x00, 0x77, 0x00, 0x00}, // 124 |
    {0x00, 0x41, 0x41, 0x36, 0x08}, // 125 }
    {0x08, 0x08, 0x2A, 0x1C, 0x08}, // 126 ~
    {0x00, 0x00, 0x00, 0x00, 0x00}  // 127 del
};

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