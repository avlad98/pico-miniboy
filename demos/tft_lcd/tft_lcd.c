/**
 * pico-miniboy: Framebuffer stored as bytes (no swap needed)
 */

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/clocks.h"
#include <string.h>
#include <stdio.h>

#define TFT_WIDTH  320
#define TFT_HEIGHT 240

#define PIN_CS 17
#define PIN_SCK 18
#define PIN_MOSI 19
#define PIN_RST 20
#define PIN_DC 21
#define PIN_BL 22

// Framebuffer as uint8_t (big-endian RGB565)
static uint8_t framebuffer[TFT_WIDTH * TFT_HEIGHT * 2];

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

void tft_swap_buffer() {
    tft_cmd(0x2A); 
    tft_data(0); tft_data(0); 
    tft_data((TFT_WIDTH-1) >> 8); tft_data((TFT_WIDTH-1) & 0xFF);
    
    tft_cmd(0x2B); 
    tft_data(0); tft_data(0); 
    tft_data((TFT_HEIGHT-1) >> 8); tft_data((TFT_HEIGHT-1) & 0xFF);
    
    tft_cmd(0x2C);
    gpio_put(PIN_DC, 1); 
    gpio_put(PIN_CS, 0);
    
    // Keep CS low the entire time, send full buffer
    uint8_t *ptr = framebuffer;
    size_t len = TFT_WIDTH * TFT_HEIGHT * 2;
    
    spi_write_blocking(spi0, ptr, len);
    
    gpio_put(PIN_CS, 1);
}

// Ultra-fast clear with 32-bit writes (correct byte order)
void fb_clear(uint16_t color) {
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    
    // Little-endian: writes lo, hi, lo, hi to memory
    uint32_t color32 = (lo << 24) | (hi << 16) | (lo << 8) | hi;
    
    uint32_t *ptr32 = (uint32_t*)framebuffer;
    int count = (TFT_WIDTH * TFT_HEIGHT * 2) / 4;
    
    while (count--) {
        *ptr32++ = color32;
    }
}


void fb_set_pixel(int x, int y, uint16_t color) {
    if (x >= 0 && x < TFT_WIDTH && y >= 0 && y < TFT_HEIGHT) {
        int idx = (y * TFT_WIDTH + x) * 2;
        framebuffer[idx] = color >> 8;
        framebuffer[idx + 1] = color & 0xFF;
    }
}

void fb_fill_rect(int x, int y, int w, int h, uint16_t color) {
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    
    for (int row = 0; row < h; row++) {
        int py = y + row;
        if (py >= 0 && py < TFT_HEIGHT) {
            for (int col = 0; col < w; col++) {
                int px = x + col;
                if (px >= 0 && px < TFT_WIDTH) {
                    int idx = (py * TFT_WIDTH + px) * 2;
                    framebuffer[idx] = hi;
                    framebuffer[idx + 1] = lo;
                }
            }
        }
    }
}

void fb_fill_circle(int cx, int cy, int radius, uint16_t color) {
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    int r2 = radius * radius;
    
    for (int y = -radius; y <= radius; y++) {
        int py = cy + y;
        if (py >= 0 && py < TFT_HEIGHT) {
            int x_extent = 0;
            while (x_extent * x_extent + y * y <= r2) x_extent++;
            
            int x_start = cx - x_extent;
            int x_end = cx + x_extent;
            
            if (x_start < 0) x_start = 0;
            if (x_end >= TFT_WIDTH) x_end = TFT_WIDTH - 1;
            
            for (int x = x_start; x <= x_end; x++) {
                int idx = (py * TFT_WIDTH + x) * 2;
                framebuffer[idx] = hi;
                framebuffer[idx + 1] = lo;
            }
        }
    }
}

const uint8_t font5x7[10][7] = {
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
    {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F},
    {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E},
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}
};

void fb_draw_digit(int x, int y, int digit, uint16_t fg, uint16_t bg) {
    for (int row = 0; row < 7; row++) {
        uint8_t bits = font5x7[digit][row];
        for (int col = 0; col < 5; col++) {
            uint16_t c = (bits & (1 << (4 - col))) ? fg : bg;
            fb_fill_rect(x + col*3, y + row*3, 3, 3, c);
        }
    }
}

void fb_draw_fps(int fps) {
    fb_fill_rect(10, 5, 100, 25, 0x0000);
    
    if (fps >= 100) {
        fb_draw_digit(12, 8, fps / 100, 0xFFFF, 0x0000);
        fb_draw_digit(30, 8, (fps / 10) % 10, 0xFFFF, 0x0000);
        fb_draw_digit(48, 8, fps % 10, 0xFFFF, 0x0000);
    } else {
        fb_draw_digit(15, 8, (fps / 10) % 10, 0xFFFF, 0x0000);
        fb_draw_digit(33, 8, fps % 10, 0xFFFF, 0x0000);
    }
}

int main() {
    set_sys_clock_khz(270000, true);
    
    uint32_t freq = clock_get_hz(clk_sys);
    clock_configure(clk_peri,
                    0,
                    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                    freq,
                    freq);
    
    stdio_init_all();
    sleep_ms(2000);
    
    gpio_init(PIN_CS); gpio_set_dir(PIN_CS, GPIO_OUT); gpio_put(PIN_CS, 1);
    gpio_init(PIN_RST); gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_init(PIN_DC); gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_init(PIN_BL); gpio_set_dir(PIN_BL, GPIO_OUT);

    spi_init(spi0, 120 * 1000 * 1000);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(16, GPIO_FUNC_SPI);

    tft_init();
    
    printf("Profiling started...\n");

    uint32_t frame_count = 0;
    uint32_t fps_timer = time_us_32();
    int current_fps = 0;
    
    uint32_t clear_time = 0;
    uint32_t circle_time = 0;
    uint32_t swap_time = 0;
    
    int circle_x = 160;
    int circle_y = 120;
    int circle_vx = 4;
    int circle_vy = 3;
    int circle_radius = 20;

    while (1) {
        uint32_t t0 = time_us_32();
        fb_clear(0x0010);
        uint32_t t1 = time_us_32();
        
        circle_x += circle_vx;
        circle_y += circle_vy;
        
        if (circle_x - circle_radius <= 0 || circle_x + circle_radius >= TFT_WIDTH) {
            circle_vx = -circle_vx;
        }
        if (circle_y - circle_radius <= 0 || circle_y + circle_radius >= TFT_HEIGHT) {
            circle_vy = -circle_vy;
        }
        
        fb_fill_circle(circle_x, circle_y, circle_radius, 0xFFE0);
        uint32_t t2 = time_us_32();
        
        fb_draw_fps(current_fps);
        
        tft_swap_buffer();
        uint32_t t3 = time_us_32();

        frame_count++;
        
        clear_time = t1 - t0;
        circle_time = t2 - t1;
        swap_time = t3 - t2;

        uint32_t now = time_us_32();
        if (now - fps_timer >= 1000000) {
            current_fps = frame_count;
            printf("Clear: %lu us, Circle: %lu us, Swap: %lu us | FPS: %d\n", 
                   clear_time, circle_time, swap_time, current_fps);
            frame_count = 0;
            fps_timer = now;
        }
    }
}
