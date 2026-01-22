#ifndef MINIBOY_ENGINE_H
#define MINIBOY_ENGINE_H

#include "surface.h"
#include <stdbool.h>
#include <stdint.h>

// Lifecycle callbacks for a MiniBoy Application
typedef struct {
  void (*init)(void);
  void (*update)(uint32_t dt_us);
  void (*draw)(surface_t *screen);
  const char *name;
  uint32_t target_fps;
} miniapp_desc_t;

// Engine Configuration
typedef struct {
  uint16_t width;
  uint16_t height;
  uint8_t pixel_format;        // 0=RGB565, 1=RGB444, 2=RGB332
  uint8_t performance_profile; // 0=Stable, 1=High, 2=Extreme
} engine_config_t;

// Initialize the engine (System, Display, Graphics)
bool engine_init(const engine_config_t *config);

// Run the application (This function does not return)
void engine_run(const miniapp_desc_t *app);

#endif
