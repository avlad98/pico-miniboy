#include "display_driver.h"
#include "framebuffer.h"
#include "pico/stdlib.h"
#include "profiler.h"
#include "system_config.h"
#include "transport_pio.h"
#include <stdio.h>

int main() {
  // System configuration - HIGH (190MHz CPU / 95MHz SPI)
  system_config_t sys_cfg = PERFORMANCE_HIGH;
  system_init(&sys_cfg);

  stdio_init_all();

  // 1. Setup Transport (PIO SPI)
  transport_pio_config_t t_cfg = {.pio = pio0,
                                  .sm = 0,
                                  .pin_sck = 18,
                                  .pin_mosi = 19,
                                  .pin_cs = 17,
                                  .pin_dc = 21};
  display_transport_t *transport = transport_pio_create(&t_cfg);
  transport->init(transport, sys_cfg.spi_hz_init, sys_cfg.spi_hz_fast);

  // 2. Setup Panel (ILI9341)
  display_config_t disp_cfg = {
      .transport = transport,
      .pin_rst = 20,
      .pin_bl = 22,
      .width = 320,
      .height = 240,
      .format = PIXEL_FORMAT_RGB444 // 12-bit: ~97 FPS, 4096 colors
  };
  display_init(&disp_cfg);

  // Performance tracking integration
  system_set_actual_spi_hz(sys_cfg.spi_hz_fast);

  // 3. Initialize framebuffer
  if (!framebuffer_init(320, 240, disp_cfg.format)) {
    printf("ERROR: Failed to allocate framebuffer\n");
    return -1;
  }

  profiler_init();

  printf("System Started. CPU: %u Hz, SPI: %u Hz\n",
         (unsigned int)system_get_cpu_hz(), (unsigned int)system_get_spi_hz());

  // Game state
  int circle_x = 160;
  int circle_y = 120;
  int circle_vx = 5;
  int circle_vy = 4;
  int circle_radius = 20;

  while (1) {
    uint32_t t0 = time_us_32();

    // Clear screen (Red background for BGR displays)
    framebuffer_clear(0x0010);

    // Update physics
    circle_x += circle_vx;
    circle_y += circle_vy;

    if (circle_x - circle_radius <= 0 || circle_x + circle_radius >= 320) {
      circle_vx = -circle_vx;
    }
    if (circle_y - circle_radius <= 0 || circle_y + circle_radius >= 240) {
      circle_vy = -circle_vy;
    }

    // Draw circle
    draw_circle(framebuffer_get_surface(), circle_x, circle_y, circle_radius,
                0xFFE0);

    // Draw Profiler Stats HUD
    profiler_draw();

    // Send to display (Asynchronous)
    framebuffer_swap_async();

    // Calculate Frame Time and Update Profiler
    uint32_t t_end = time_us_32();
    profiler_update(t_end - t0);
  }

  return 0;
}
