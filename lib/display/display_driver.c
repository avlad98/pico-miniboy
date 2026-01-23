#include "display_driver.h"
#include "hardware/gpio.h"

static display_config_t current_config;

void display_init(const display_config_t *config) {
  current_config = *config;
  display_transport_t *t = config->transport;

  // Reset display
  gpio_init(config->pin_rst);
  gpio_set_dir(config->pin_rst, GPIO_OUT);
  gpio_put(config->pin_rst, 0);
  sleep_ms(50);
  gpio_put(config->pin_rst, 1);
  sleep_ms(150);

  // Initial speed (slow)
  t->set_speed(t, false);

  t->send_cmd(t, 0x01); // Software reset
  sleep_ms(150);

  t->send_cmd(t, 0x11); // Sleep out
  sleep_ms(150);

  t->send_cmd(t, 0x3A); // Pixel Format
  if (config->format == PIXEL_FORMAT_RGB332) {
    t->send_data8(t, 0x55); // RGB565 for hardware expansion/LUT
  } else {
    t->send_data8(t, (uint8_t)config->format);
  }

  t->send_cmd(t, 0x36);   // Memory Access Control
  t->send_data8(t, 0x68); // Landscape, BGR

  t->send_cmd(t, 0x29); // Display ON
  sleep_ms(50);

  // Turn on backlight
  gpio_init(config->pin_bl);
  gpio_set_dir(config->pin_bl, GPIO_OUT);
  gpio_put(config->pin_bl, 1);

  // Note: Frequency tracking moved to transport or main
}

void display_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  display_transport_t *t = current_config.transport;
  t->set_speed(t, false);

  t->send_cmd(t, 0x2A);
  t->send_data8(t, x0 >> 8);
  t->send_data8(t, x0 & 0xFF);
  t->send_data8(t, x1 >> 8);
  t->send_data8(t, x1 & 0xFF);

  t->send_cmd(t, 0x2B);
  t->send_data8(t, y0 >> 8);
  t->send_data8(t, y0 & 0xFF);
  t->send_data8(t, y1 >> 8);
  t->send_data8(t, y1 & 0xFF);

  t->send_cmd(t, 0x2C);
}

void display_start_bulk(void) {
  display_transport_t *t = current_config.transport;
  t->set_speed(t, true); // Fast mode
}

void display_end_bulk(void) {
  display_transport_t *t = current_config.transport;
  t->wait(t);
  t->set_speed(t, false);
}

void display_send_buffer(const uint8_t *data, uint32_t len) {
  display_transport_t *t = current_config.transport;
  t->send_buffer(t, data, len);
}

void display_push_pixels(uint16_t color, uint32_t count) {
  display_transport_t *t = current_config.transport;
  // Assume caller has called display_start_bulk (set_speed true)

  // Create a small chunk buffer for filling
  uint8_t chunk[64];
  uint32_t chunk_capacity_pixels;
  uint32_t chunk_size_bytes;

  if (current_config.format == PIXEL_FORMAT_RGB444) {
    // RGB444: 3 bytes = 2 pixels
    // 64 bytes can hold 42 pixels (21 pairs = 63 bytes)
    // Construct 2 pixels (3 bytes)
    uint8_t r4 = ((color >> 11) & 0x1F) >> 1;
    uint8_t g4 = ((color >> 5) & 0x3F) >> 2;
    uint8_t b4 = (color & 0x1F) >> 1;
    uint8_t b0 = (r4 << 4) | g4;
    uint8_t b1 = (b4 << 4) | r4;
    uint8_t b2 = (g4 << 4) | b4;

    // Fill chunk
    for (int i = 0; i < 21; i++) {
      chunk[i * 3 + 0] = b0;
      chunk[i * 3 + 1] = b1;
      chunk[i * 3 + 2] = b2;
    }
    chunk_capacity_pixels = 42;
    chunk_size_bytes = 63;
  } else {
    // RGB565 / Expaded RGB332: 2 bytes per pixel
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    for (int i = 0; i < 32; i++) {
      chunk[i * 2] = hi;
      chunk[i * 2 + 1] = lo;
    }
    chunk_capacity_pixels = 32;
    chunk_size_bytes = 64;
  }

  while (count > 0) {
    uint32_t push_pixels =
        (count > chunk_capacity_pixels) ? chunk_capacity_pixels : count;
    uint32_t push_bytes;

    if (current_config.format == PIXEL_FORMAT_RGB444) {
      uint32_t pairs = (push_pixels + 1) / 2;
      push_bytes = pairs * 3;
    } else {
      push_bytes = push_pixels * 2;
    }

    t->send_buffer(t, chunk, push_bytes);
    t->wait(t); // CRITICAL: Wait for DMA to finish before reusing buffer/DMA!
    count -= push_pixels;
  }
}

uint16_t display_get_width(void) { return current_config.width; }
uint16_t display_get_height(void) { return current_config.height; }

bool display_is_busy(void) {
  display_transport_t *t = current_config.transport;
  if(t && t->is_busy) return t->is_busy(t);
  return false;
}
