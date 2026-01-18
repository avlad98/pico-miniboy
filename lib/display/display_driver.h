#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "system_config.h"

typedef enum {
    PIXEL_FORMAT_RGB565 = 0x55,
    PIXEL_FORMAT_RGB444 = 0x53
} display_pixel_format_t;

typedef struct {
    spi_inst_t *spi;
    uint8_t pin_cs;
    uint8_t pin_sck;
    uint8_t pin_mosi;
    uint8_t pin_miso;
    uint8_t pin_rst;
    uint8_t pin_dc;
    uint8_t pin_bl;
    uint16_t width;
    uint16_t height;
    display_pixel_format_t format;
} display_config_t;

// Initialize display hardware (uses system_config for SPI speeds)
void display_init(const display_config_t *config, const system_config_t *sys_config);

// Low-level display commands
void display_cmd(uint8_t cmd);
void display_data(uint8_t data);
void display_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void display_start_write(void);
void display_end_write(void);

// Get dimensions
uint16_t display_get_width(void);
uint16_t display_get_height(void);

// Get SPI instance
spi_inst_t* display_get_spi(void);

#endif
