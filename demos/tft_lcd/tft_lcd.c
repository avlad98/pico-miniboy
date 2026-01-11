/**
 * pico-miniboy: Landscape with correct digits 0-9
 */

#include "pico/stdlib.h"
#include "hardware/spi.h"

#define TFT_WIDTH  320
#define TFT_HEIGHT 240

#define PIN_CS 17
#define PIN_SCK 18
#define PIN_MOSI 19
#define PIN_RST 20
#define PIN_DC 21
#define PIN_BL 22

void tft_cmd(uint8_t c) {
    gpio_put(PIN_DC, 0); 
    gpio_put(PIN_CS, 0);
    spi_write_blocking(spi0, &c, 1);
    gpio_put(PIN_CS, 1);
}

void tft_data(uint8_t d) {
    gpio_put(PIN_DC, 1); 
    gpio_put(PIN_CS, 0);
    spi_write_blocking(spi0, &d, 1);
    gpio_put(PIN_CS, 1);
}

void tft_init() {
    gpio_put(PIN_RST, 0); sleep_ms(10);
    gpio_put(PIN_RST, 1); sleep_ms(120);
    
    tft_cmd(0x01); sleep_ms(150);
    tft_cmd(0x11); sleep_ms(10);
    tft_cmd(0x36); tft_data(0x68);
    tft_cmd(0x3A); tft_data(0x55);
    tft_cmd(0x29);
    
    gpio_put(PIN_BL, 1);
}

void tft_fill_rect(int x, int y, int w, int h, uint16_t color) {
    tft_cmd(0x2A); 
    tft_data(x >> 8); tft_data(x); 
    tft_data((x+w-1) >> 8); tft_data(x+w-1);
    
    tft_cmd(0x2B); 
    tft_data(y >> 8); tft_data(y); 
    tft_data((y+h-1) >> 8); tft_data(y+h-1);
    
    tft_cmd(0x2C);
    gpio_put(PIN_DC, 1); 
    gpio_put(PIN_CS, 0);
    
    uint16_t pixel = (color >> 8) | (color << 8);
    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
        spi_write_blocking(spi0, (uint8_t*)&pixel, 2);
    }
    gpio_put(PIN_CS, 1);
}

// 5x7 font for digits 0-9
const uint8_t font5x7[10][7] = {
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 0
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 1
    {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}, // 2
    {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}, // 3
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 4
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, // 5
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, // 6
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 7
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 8
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}  // 9
};

void draw_digit(int x, int y, int digit, uint16_t fg, uint16_t bg) {
    for (int row = 0; row < 7; row++) {
        uint8_t bits = font5x7[digit][row];
        for (int col = 0; col < 5; col++) {
            uint16_t c = (bits & (1 << (4 - col))) ? fg : bg;
            tft_fill_rect(x + col*3, y + row*3, 3, 3, c);
        }
    }
}

int main() {
    gpio_init(PIN_CS); gpio_set_dir(PIN_CS, GPIO_OUT); gpio_put(PIN_CS, 1);
    gpio_init(PIN_RST); gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_init(PIN_DC); gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_init(PIN_BL); gpio_set_dir(PIN_BL, GPIO_OUT);

    spi_init(spi0, 20 * 1000 * 1000);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(16, GPIO_FUNC_SPI);

    tft_init();

    // Black background
    tft_fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, 0x0000);

    // Draw digits 0-9 horizontally from left to right
    for (int i = 0; i < 10; i++) {
        draw_digit(15 + i * 30, 25, i, 0xFFFF, 0x0000);
    }

    while (1) {
        tight_loop_contents();
    }
}
