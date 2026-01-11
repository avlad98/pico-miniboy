#include "font.h"
#include "framebuffer.h"

// 5x7 digit font data (0-9)
static const uint8_t font_5x7_data[10][7] = {
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 0
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 1
    {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}, // 2
    {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}, // 3
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 4
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, // 5
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, // 6
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 7
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 8
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}  // 9
};

const font_t font_5x7 = {
    .data = (const uint8_t*)font_5x7_data,
    .width = 5,
    .height = 7,
    .scale = 3
};

void font_draw_char(int x, int y, char c, uint16_t fg, uint16_t bg, const font_t *font) {
    // Only support digits 0-9
    if (c < '0' || c > '9') return;
    
    int digit = c - '0';
    const uint8_t *glyph = &font->data[digit * font->height];
    
    for (int row = 0; row < font->height; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < font->width; col++) {
            uint16_t color = (bits & (1 << (4 - col))) ? fg : bg;
            framebuffer_fill_rect(
                x + col * font->scale, 
                y + row * font->scale, 
                font->scale, 
                font->scale, 
                color
            );
        }
    }
}

void font_draw_string(int x, int y, const char *str, uint16_t fg, uint16_t bg, const font_t *font) {
    int cursor_x = x;
    while (*str) {
        font_draw_char(cursor_x, y, *str, fg, bg, font);
        cursor_x += (font->width + 1) * font->scale;  // +1 for spacing
        str++;
    }
}

void font_draw_number(int x, int y, int num, uint16_t fg, uint16_t bg, const font_t *font) {
    // Clear area first
    framebuffer_fill_rect(x, y, 100, 25, bg);
    
    // Convert number to string and draw
    char buffer[16];
    int len = 0;
    
    if (num == 0) {
        buffer[len++] = '0';
    } else {
        int temp = num;
        int digits = 0;
        while (temp > 0) {
            digits++;
            temp /= 10;
        }
        
        for (int i = digits - 1; i >= 0; i--) {
            buffer[i] = '0' + (num % 10);
            num /= 10;
        }
        len = digits;
    }
    buffer[len] = '\0';
    
    font_draw_string(x, y, buffer, fg, bg, font);
}
