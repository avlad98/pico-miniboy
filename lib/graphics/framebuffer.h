#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdbool.h>
#include <stdint.h>

#include "display_driver.h"
#include "surface.h"

// Initialize framebuffer
bool framebuffer_init(uint16_t width, uint16_t height,
                      display_pixel_format_t format);

// Get the active drawing surface
surface_t *framebuffer_get_surface(void);

// Primitive drawing on a surface
void draw_clear(surface_t *surf, uint16_t color);
void draw_pixel(surface_t *surf, int x, int y, uint16_t color);
void draw_rect(surface_t *surf, int x, int y, int w, int h, uint16_t color);
void draw_circle(surface_t *surf, int cx, int cy, int radius, uint16_t color);

// High-level framebuffer operations
void framebuffer_clear(uint16_t color);
void framebuffer_fill_circle(int cx, int cy, int radius, uint16_t color);
void framebuffer_swap_async(void);
void framebuffer_wait_last_swap(void);

// Performance & Profiling
uint32_t framebuffer_get_last_wait_time(void);
void framebuffer_reset_profile_stats(void);

#endif
