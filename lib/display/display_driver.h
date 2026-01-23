#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include "display_transport.h"
#include "pico/stdlib.h"

typedef enum {
  PIXEL_FORMAT_RGB565 = 0x55, // 16-bit: 65,536 colors
  PIXEL_FORMAT_RGB444 = 0x53, // 12-bit: 4,096 colors
  PIXEL_FORMAT_RGB332 = 0x52  // 8-bit: 256 colors
} display_pixel_format_t;

typedef struct {
  display_transport_t *transport;
  uint8_t pin_rst;
  uint8_t pin_bl;
  uint16_t width;
  uint16_t height;
  display_pixel_format_t format;
} display_config_t;

// Initialize display hardware
void display_init(const display_config_t *config);

// Panel abstraction
void display_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void display_start_bulk(void);
void display_end_bulk(void);
void display_send_buffer(const uint8_t *data, uint32_t len);
void display_push_pixels(uint16_t color, uint32_t count);

// Get dimensions
uint16_t display_get_width(void);
uint16_t display_get_height(void);
bool display_is_busy(void);

#endif
