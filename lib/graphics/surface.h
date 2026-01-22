#ifndef SURFACE_H
#define SURFACE_H

#include "display_driver.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * Surface structure
 * Defines a memory region where pixels can be drawn.
 */
typedef struct {
  uint8_t *pixels;
  uint16_t width;
  uint16_t height;
  display_pixel_format_t format;
  uint32_t size;
} surface_t;

#endif
