#ifndef MINIBOY_ENGINE_H
#define MINIBOY_ENGINE_H

#include "display_driver.h"
#include "surface.h"

// Lifecycle callbacks for a MiniBoy Application
typedef struct {
  void (*init)(void);
  void (*update)(uint32_t dt_us);
  void (*draw)(surface_t *screen);
  const char *name;
  uint32_t target_fps;
} miniapp_desc_t;

// Performance Profiles
typedef enum {
  PROFILE_STABLE = 0,
  PROFILE_BALANCED,
  PROFILE_TURBO,
  PROFILE_HIGH,
  PROFILE_MAX,
  PROFILE_EXTREME
} engine_profile_t;

// Engine Configuration
typedef struct {
  uint16_t width;
  uint16_t height;
  display_pixel_format_t pixel_format; // Use PIXEL_FORMAT_* from display_driver.h
  engine_profile_t performance_profile; // Use PROFILE_* enum
  uint8_t buffer_count;
} engine_config_t;

// Initialize the engine (System, Display, Graphics)
bool engine_init(const engine_config_t *config);

// Run the application (This function does not return)
void engine_run(const miniapp_desc_t *app);

#endif
