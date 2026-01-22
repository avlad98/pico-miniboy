#include "miniboy_engine.h"
#include "display_driver.h"
#include "framebuffer.h"
#include "pico/stdlib.h"
#include "profiler.h"
#include "system_config.h"
#include "transport_pio.h"
#include <stdio.h>

static display_transport_t *transport = NULL;
static engine_config_t engine_config;

bool engine_init(const engine_config_t *config) {
  engine_config = *config;
  // 1. System Configuration
  const system_config_t *sys_cfg;
  // Use the profile index directly since we now use an enum matching the array
  if (config->performance_profile >= 0 &&
      config->performance_profile < 6) { // 6 profiles in system_config.c
    sys_cfg = &system_profiles[config->performance_profile];
  } else {
    sys_cfg = &system_profiles[3]; // Default HIGH
  }

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
  display_pixel_format_t fmt = config->pixel_format;

  display_config_t disp_cfg = {.transport = transport,
                               .pin_rst = 20,
                               .pin_bl = 22,
                               .width = config->width,
                               .height = config->height,
                               .format = fmt};
  display_init(&disp_cfg);

  // 4. Graphics Engine
  system_set_actual_spi_hz(sys_cfg->spi_hz_fast);
  uint8_t bufs = config->buffer_count;

  if (!framebuffer_init(config->width, config->height, fmt, bufs)) {
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

    // 6. Single-Buffer Synchronization
    // If we only have 1 buffer, we cannot start drawing the next frame
    // until the current one has finished sending to the display, otherwise
    // we will overwrite data as it is being sent (Tearing/Corruption).
    if (engine_config.buffer_count == 1) {
      framebuffer_wait_last_swap();
    }

    // 5. Stats (Updated after sync to capture real frame time)
    uint32_t t_end = time_us_32();
    profiler_update(t_end - t0);
  }
}
