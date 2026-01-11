#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include <stdbool.h>

// Initialize framebuffer (allocates memory)
bool framebuffer_init(uint16_t width, uint16_t height);

// Get framebuffer info
uint8_t* framebuffer_get_buffer(void);
uint32_t framebuffer_get_size(void);

// Drawing primitives
void framebuffer_clear(uint16_t color);
void framebuffer_set_pixel(int x, int y, uint16_t color);
void framebuffer_fill_rect(int x, int y, int w, int h, uint16_t color);
void framebuffer_fill_circle(int cx, int cy, int radius, uint16_t color);

// Send framebuffer to display
void framebuffer_swap(void);

#endif
