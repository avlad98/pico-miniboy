#include "miniboy_engine.h"
#include "display_driver.h"
#include "framebuffer.h"
#include "pico/stdlib.h"
#include "profiler.h"
#include "system_config.h"
#include "transport_pio.h"
#include <stdio.h>

static display_transport_t *transport = NULL;

bool engine_init(const engine_config_t *config) {
  // 1. System Configuration
  const system_config_t *sys_cfg;
  if (config->performance_profile == 0)
    sys_cfg = &system_profiles[0]; // STABLE
  else if (config->performance_profile == 2)
    sys_cfg = &system_profiles[5]; // EXTREME
  else
    sys_cfg = &system_profiles[3]; // HIGH (Default)

  system_init(sys_cfg);
  stdio_init_all();

  // 2. Transport Configuration (PIO SPI)
  // TODO: Move pin mapping to board_config.h
  transport_pio_config_t t_cfg = {.pio = pio0,
                                  .sm = 0,
                                  .pin_sck = 18,
                                  .pin_mosi = 19,
                                  .pin_cs = 17,
                                  .pin_dc = 21};
  transport = transport_pio_create(&t_cfg);
  if (!transport)
    return false;

  transport->init(transport, sys_cfg->spi_hz_init, sys_cfg->spi_hz_fast);

  // 3. Display Configuration
  display_pixel_format_t fmt;
  if (config->pixel_format == 1)
    fmt = PIXEL_FORMAT_RGB444;
  else if (config->pixel_format == 2)
    fmt = PIXEL_FORMAT_RGB332;
  else
    fmt = PIXEL_FORMAT_RGB565;

  display_config_t disp_cfg = {.transport = transport,
                               .pin_rst = 20,
                               .pin_bl = 22,
                               .width = config->width,
                               .height = config->height,
                               .format = fmt};
  display_init(&disp_cfg);

  // 4. Graphics Engine
  system_set_actual_spi_hz(sys_cfg->spi_hz_fast);
  if (!framebuffer_init(config->width, config->height, fmt)) {
    printf("CORE: Framebuffer allocation failed!\n");
    return false;
  }

  profiler_init();
  return true;
}

void engine_run(const miniapp_desc_t *app) {
  printf("CORE: Starting Application: %s\n", app->name ? app->name : "Unknown");

  if (app->init)
    app->init();

  uint32_t last_time = time_us_32();

  while (true) {
    uint32_t t0 = time_us_32();

    // 1. Update Phase
    if (app->update) {
      uint32_t dt = t0 - last_time;
      app->update(dt);
    }
    last_time = t0;

    // 2. Draw Phase
    if (app->draw) {
      app->draw(framebuffer_get_surface());
    }

    // 3. System Overlays
    profiler_draw();

    // 4. Present
    framebuffer_swap_async();

    // 5. Stats
    uint32_t t_end = time_us_32();
    profiler_update(t_end - t0);
  }
}
