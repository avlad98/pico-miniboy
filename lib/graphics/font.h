#ifndef FONT_H
#define FONT_H

#include "surface.h"

typedef struct {
  const uint8_t *data;
  uint8_t width;
  uint8_t height;
  uint8_t scale;
} font_t;

// Built-in 5x7 font
extern const font_t font_5x7;

// Drawing functions
void font_draw_char(surface_t *surf, int x, int y, char c, uint16_t fg,
                    uint16_t bg, const font_t *font);
void font_draw_string(surface_t *surf, int x, int y, const char *str,
                      uint16_t fg, uint16_t bg, const font_t *font);
void font_draw_number(surface_t *surf, int x, int y, int num, uint16_t fg,
                      uint16_t bg, const font_t *font);

#endif
