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
  t->set_speed(t, true);
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

uint16_t display_get_width(void) { return current_config.width; }
uint16_t display_get_height(void) { return current_config.height; }
