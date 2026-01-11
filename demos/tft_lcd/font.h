#ifndef FONT_H
#define FONT_H

#include <stdint.h>

typedef struct {
    const uint8_t *data;
    uint8_t width;
    uint8_t height;
    uint8_t scale;
} font_t;

// Built-in 5x7 font
extern const font_t font_5x7;

// Drawing functions
void font_draw_char(int x, int y, char c, uint16_t fg, uint16_t bg, const font_t *font);
void font_draw_string(int x, int y, const char *str, uint16_t fg, uint16_t bg, const font_t *font);
void font_draw_number(int x, int y, int num, uint16_t fg, uint16_t bg, const font_t *font);

#endif
