#include "pico/stdlib.h"
#include "hardware/spi.h"

#define USE_TFT 1  // Toggle: 0=LED only, 1=TFT+LED

#define LED_PIN 25
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// TFT pins
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19
#define PIN_RST  20
#define PIN_DC   21

#if USE_TFT
void tft_cmd(uint8_t cmd) {
    gpio_put(PIN_DC, 0);
    gpio_put(PIN_CS, 0);
    spi_write_blocking(spi0, &cmd, 1);
    gpio_put(PIN_CS, 1);
}

void tft_data(uint8_t data) {
    gpio_put(PIN_DC, 1);
    gpio_put(PIN_CS, 0);
    spi_write_blocking(spi0, &data, 1);
    gpio_put(PIN_CS, 1);
}

void tft_init() {
    gpio_put(PIN_RST, 0); sleep_ms(100);
    gpio_put(PIN_RST, 1); sleep_ms(100);

    tft_cmd(0x01); sleep_ms(150);
    tft_cmd(0x11); sleep_ms(150);
    tft_cmd(0x36); tft_data(0x48);
    tft_cmd(0x3A); tft_data(0x55);
    tft_cmd(0xB1); tft_data(0x00); tft_data(0x18);
    tft_cmd(0xC0); tft_data(0x21);
    tft_cmd(0xC1); tft_data(0x12);
    tft_cmd(0xC7); tft_data(0x20);
    tft_cmd(0x21);
    tft_cmd(0x2A); tft_data(0x00); tft_data(0x00); tft_data(0x00); tft_data(0xEF);
    tft_cmd(0x2B); tft_data(0x00); tft_data(0x00); tft_data(0x01); tft_data(0x3F);
    tft_cmd(0x29);
}

uint8_t tft_progress = 0;  // 0-255 progress indicator
void tft_fill_screen_nonblock(uint16_t color) {
    static uint32_t pixel_count = 0;
    static uint16_t pixel = 0;

    if (pixel_count == 0) {
        // Start new fill
        tft_cmd(0x2C);
        gpio_put(PIN_DC, 1);
        gpio_put(PIN_CS, 0);
        pixel = (color >> 8) | (color << 8);
    }

    // Fill 512 pixels per call (~0.1ms, non-blocking)
    for (int i = 0; i < 256; ++i) {
        if (pixel_count >= (uint32_t)TFT_WIDTH * TFT_HEIGHT) break;
        spi_write_blocking(spi0, (uint8_t*)&pixel, 2);
        pixel_count++;
        tft_progress = (pixel_count * 255) / (TFT_WIDTH * TFT_HEIGHT);
    }

    if (pixel_count >= (uint32_t)TFT_WIDTH * TFT_HEIGHT) {
        gpio_put(PIN_CS, 1);
        pixel_count = 0;
    }
}
#endif

int main() {
    uint32_t led_time = 0;
    uint32_t tft_time = 0;
    int color_idx = 0;
    uint16_t colors[] = {0xF800, 0x07E0, 0x001F, 0xFFFF, 0xF81F};
    bool led_state = false;  // Add this

    // Always init LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

#if USE_TFT
    // TFT pins
    gpio_init(PIN_CS); gpio_set_dir(PIN_CS, GPIO_OUT); gpio_put(PIN_CS, 1);
    gpio_init(PIN_RST); gpio_set_dir(PIN_RST, GPIO_OUT); gpio_put(PIN_RST, 1);
    gpio_init(PIN_DC); gpio_set_dir(PIN_DC, GPIO_OUT);

    spi_init(spi0, 10 * 1000 * 1000);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    tft_init();
#endif

    while (1) {
        uint32_t now = time_us_32();

        // LED blink (fixed)
        if ((now - led_time) > 250000) {
            led_state = !led_state;  // Toggle
            gpio_put(LED_PIN, led_state);
            led_time = now;
        }

#if USE_TFT
        // TFT color cycle (every 10s, non-blocking fill)
        if ((now - tft_time) > 10000000) {  // 10s per color
            tft_fill_screen_nonblock(colors[color_idx]);
            color_idx = (color_idx + 1) % 5;
            tft_time = now;
        } else {
            // Continue current fill
            tft_fill_screen_nonblock(colors[color_idx]);
        }
#else
        // LED-only: nothing extra
#endif

        tight_loop_contents();  // Low power
    }
}
