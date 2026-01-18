#include "display_driver.h"
#include "hardware/gpio.h"

static display_config_t current_display;

void display_init(const display_config_t *config, const system_config_t *sys_config) {
    current_display = *config;
    
    // GPIO setup
    gpio_init(config->pin_cs);
    gpio_set_dir(config->pin_cs, GPIO_OUT);
    gpio_put(config->pin_cs, 1);
    
    gpio_init(config->pin_rst);
    gpio_set_dir(config->pin_rst, GPIO_OUT);
    
    gpio_init(config->pin_dc);
    gpio_set_dir(config->pin_dc, GPIO_OUT);
    
    gpio_init(config->pin_bl);
    gpio_set_dir(config->pin_bl, GPIO_OUT);
    
    // Init SPI at slow speed for display init
    spi_init(config->spi, sys_config->spi_hz_init);
    gpio_set_function(config->pin_sck, GPIO_FUNC_SPI);
    gpio_set_function(config->pin_mosi, GPIO_FUNC_SPI);
    gpio_set_function(config->pin_miso, GPIO_FUNC_SPI);
    
    // Improve signal integrity for high-speed SPI
    gpio_set_drive_strength(config->pin_sck, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_slew_rate(config->pin_sck, GPIO_SLEW_RATE_FAST);
    gpio_set_drive_strength(config->pin_mosi, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_slew_rate(config->pin_mosi, GPIO_SLEW_RATE_FAST);
    
    // Hardware reset
    gpio_put(config->pin_rst, 0);
    sleep_ms(10);
    gpio_put(config->pin_rst, 1);
    sleep_ms(120);
    
    // ILI9341 init sequence
    display_cmd(0x01); // Software reset
    sleep_ms(150);
    
    display_cmd(0x11); // Sleep out
    sleep_ms(10);
    
    display_cmd(0x36); // Memory Access Control
    display_data(0x68);
    
    display_cmd(0x3A); // Pixel Format
    display_data(0x55); // 16-bit RGB565
    
    display_cmd(0x29); // Display ON
    
    // Speed up SPI for framebuffer transfers
    uint32_t actual = spi_set_baudrate(config->spi, sys_config->spi_hz_fast);
    system_set_actual_spi_hz(actual);
    
    // Backlight on
    gpio_put(config->pin_bl, 1);
}

void display_cmd(uint8_t cmd) {
    gpio_put(current_display.pin_dc, 0);
    gpio_put(current_display.pin_cs, 0);
    spi_write_blocking(current_display.spi, &cmd, 1);
    gpio_put(current_display.pin_cs, 1);
}

void display_data(uint8_t data) {
    gpio_put(current_display.pin_dc, 1);
    gpio_put(current_display.pin_cs, 0);
    spi_write_blocking(current_display.spi, &data, 1);
    gpio_put(current_display.pin_cs, 1);
}

void display_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Column address
    display_cmd(0x2A);
    display_data(x0 >> 8);
    display_data(x0 & 0xFF);
    display_data(x1 >> 8);
    display_data(x1 & 0xFF);
    
    // Row address
    display_cmd(0x2B);
    display_data(y0 >> 8);
    display_data(y0 & 0xFF);
    display_data(y1 >> 8);
    display_data(y1 & 0xFF);
    
    // Write to RAM
    display_cmd(0x2C);
}

void display_start_write(void) {
    gpio_put(current_display.pin_dc, 1);
    gpio_put(current_display.pin_cs, 0);
}

void display_end_write(void) {
    gpio_put(current_display.pin_cs, 1);
}

uint16_t display_get_width(void) {
    return current_display.width;
}

uint16_t display_get_height(void) {
    return current_display.height;
}

spi_inst_t* display_get_spi(void) {
    return current_display.spi;
}
