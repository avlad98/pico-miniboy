#include "framebuffer.h"
#include "display_driver.h"
#include "dma_spi.h"
#include <string.h>
#include <stdlib.h>

static uint8_t *framebuffer = NULL;
static uint16_t fb_width = 0;
static uint16_t fb_height = 0;
static uint32_t fb_size = 0;
static display_pixel_format_t fb_format;

bool framebuffer_init(uint16_t width, uint16_t height, display_pixel_format_t format) {
    fb_width = width;
    fb_height = height;
    fb_format = format;
    
    if (format == PIXEL_FORMAT_RGB565) {
        fb_size = width * height * 2;
    } else {
        fb_size = (width * height * 3) / 2;
    }
    
    framebuffer = (uint8_t*)malloc(fb_size);
    if (framebuffer == NULL) {
        return false;
    }
    
    return true;
}

uint8_t* framebuffer_get_buffer(void) {
    return framebuffer;
}

uint32_t framebuffer_get_size(void) {
    return fb_size;
}

void framebuffer_clear(uint16_t color) {
    if (fb_format == PIXEL_FORMAT_RGB565) {
        uint8_t hi = color >> 8;
        uint8_t lo = color & 0xFF;
        uint32_t color32 = (lo << 24) | (hi << 16) | (lo << 8) | hi;
        
        uint32_t *ptr32 = (uint32_t*)framebuffer;
        uint32_t *end32 = (uint32_t*)(framebuffer + fb_size);
        while (ptr32 < end32) *ptr32++ = color32;
    } else {
        // RGB444: 3 bytes for 2 pixels. 
        // Pixel 1: [R1 G1] [B1 R2] [G2 B2]
        uint8_t r = (color >> 11) & 0x1F; // 5 bits to 4
        uint8_t g = (color >> 5) & 0x3F;  // 6 bits to 4
        uint8_t b = (color & 0x1F);      // 5 bits to 4
        uint8_t r4 = r >> 1; uint8_t g4 = g >> 2; uint8_t b4 = b >> 1;
        
        uint16_t p1 = (r4 << 8) | (g4 << 4) | b4;
        uint8_t b0 = (p1 >> 4) & 0xFF; // R G
        uint8_t b1 = ((p1 & 0x0F) << 4) | (r4); // B R
        uint8_t b2 = (g4 << 4) | b4; // G B
        
        for (int i = 0; i < fb_size; i += 3) {
            framebuffer[i] = b0;
            framebuffer[i+1] = b1;
            framebuffer[i+2] = b2;
        }
    }
}

void framebuffer_set_pixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= fb_width || y < 0 || y >= fb_height) return;
    
    if (fb_format == PIXEL_FORMAT_RGB565) {
        int idx = (y * fb_width + x) * 2;
        framebuffer[idx] = color >> 8;
        framebuffer[idx + 1] = color & 0xFF;
    } else {
        // RGB444: 3 bytes for 2 pixels
        int pixel_idx = y * fb_width + x;
        int byte_idx = (pixel_idx / 2) * 3;
        
        uint8_t r4 = ((color >> 11) & 0x1F) >> 1;
        uint8_t g4 = ((color >> 5) & 0x3F) >> 2;
        uint8_t b4 = (color & 0x1F) >> 1;
        
        if (pixel_idx % 2 == 0) {
            // Even pixel: [R1 G1] [B1 ..]
            framebuffer[byte_idx] = (r4 << 4) | g4;
            framebuffer[byte_idx+1] = (b4 << 4) | (framebuffer[byte_idx+1] & 0x0F);
        } else {
            // Odd pixel: [.. ..] [.. R2] [G2 B2]
            framebuffer[byte_idx+1] = (framebuffer[byte_idx+1] & 0xF0) | r4;
            framebuffer[byte_idx+2] = (g4 << 4) | b4;
        }
    }
}

void framebuffer_fill_rect(int x, int y, int w, int h, uint16_t color) {
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    
    for (int row = 0; row < h; row++) {
        int py = y + row;
        if (py >= 0 && py < fb_height) {
            for (int col = 0; col < w; col++) {
                int px = x + col;
                if (px >= 0 && px < fb_width) {
                    int idx = (py * fb_width + px) * 2;
                    framebuffer[idx] = hi;
                    framebuffer[idx + 1] = lo;
                }
            }
        }
    }
}

void framebuffer_fill_circle(int cx, int cy, int radius, uint16_t color) {
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        // Draw horizontal lines to fill the circle
        for (int i = cx - x; i <= cx + x; i++) {
            framebuffer_set_pixel(i, cy + y, color);
            framebuffer_set_pixel(i, cy - y, color);
        }
        for (int i = cx - y; i <= cx + y; i++) {
            framebuffer_set_pixel(i, cy + x, color);
            framebuffer_set_pixel(i, cy - x, color);
        }

        y++;
        err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) {
            x--;
            err += 1 - 2 * x;
        }
    }
}

static bool dma_active = false;

void framebuffer_swap(void) {
    framebuffer_swap_async();
    dma_spi_wait();
    display_end_write();
    dma_active = false;
}

void framebuffer_swap_async(void) {
    // Note: Do NOT wait here unless necessary, we wait at the start of the next frame
    display_set_window(0, 0, fb_width - 1, fb_height - 1);
    display_start_write();
    dma_spi_transfer(framebuffer, fb_size);
    dma_active = true;
}

void framebuffer_wait_last_swap(void) {
    if (dma_active) {
        dma_spi_wait();
        display_end_write();
        dma_active = false;
    }
}
