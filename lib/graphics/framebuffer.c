#include "framebuffer.h"
#include "display_driver.h"
#include "dma_spi.h"
#include <string.h>
#include <stdlib.h>

static uint8_t *framebuffer_v[2] = {NULL, NULL};
static uint8_t back_buffer_idx = 0;
static uint16_t fb_width = 0;
static uint16_t fb_height = 0;
static uint32_t fb_size = 0;
static display_pixel_format_t fb_format;
#define framebuffer framebuffer_v[back_buffer_idx]

bool framebuffer_init(uint16_t width, uint16_t height, display_pixel_format_t format) {
    fb_width = width;
    fb_height = height;
    fb_format = format;
    
    if (format == PIXEL_FORMAT_RGB565) {
        fb_size = width * height * 2;
    } else {
        fb_size = (width * height * 3) / 2;
    }
    
    framebuffer_v[0] = (uint8_t*)malloc(fb_size);
    // Only double-buffer if we have enough RAM (RGB444 mode)
    if (format == PIXEL_FORMAT_RGB444) {
        framebuffer_v[1] = (uint8_t*)malloc(fb_size);
    } else {
        framebuffer_v[1] = framebuffer_v[0]; // Point to same for 565
    }

    if (framebuffer_v[0] == NULL || framebuffer_v[1] == NULL) {
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
        uint8_t r4 = ((color >> 11) & 0x1F) >> 1;
        uint8_t g4 = ((color >> 5) & 0x3F) >> 2;
        uint8_t b4 = (color & 0x1F) >> 1;
        
        // 3 bytes for 2 pixels: [R1 G1] [B1 R2] [G2 B2]
        uint8_t b0 = (r4 << 4) | g4;
        uint8_t b1 = (b4 << 4) | r4;
        uint8_t b2 = (g4 << 4) | b4;
        
        // Optimize using 32-bit writes. Pattern repeats every 12 bytes (3 words)
        // [b0 b1 b2 b0] [b1 b2 b0 b1] [b2 b0 b1 b2]
        uint32_t w0 = b0 | (b1 << 8) | (b2 << 16) | (b0 << 24);
        uint32_t w1 = b1 | (b2 << 8) | (b0 << 16) | (b1 << 24);
        uint32_t w2 = b2 | (b0 << 8) | (b1 << 16) | (b2 << 24);
        
        uint32_t *ptr32 = (uint32_t*)framebuffer;
        uint32_t *end32 = (uint32_t*)(framebuffer + fb_size);
        
        while (ptr32 < end32) {
            *ptr32++ = w0;
            *ptr32++ = w1;
            *ptr32++ = w2;
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
    if (fb_format == PIXEL_FORMAT_RGB565) {
        uint8_t hi = color >> 8;
        uint8_t lo = color & 0xFF;
        for (int row = 0; row < h; row++) {
            int py = y + row;
            if (py >= 0 && py < fb_height) {
                uint8_t *ptr = &framebuffer[(py * fb_width + x) * 2];
                for (int col = 0; col < w; col++) {
                    *ptr++ = hi;
                    *ptr++ = lo;
                }
            }
        }
    } else {
        // Optimizing 12-bit fill is harder because of alignment, 
        // but we can optimize the common case where x and w are even.
        for (int row = 0; row < h; row++) {
            int py = y + row;
            if (py >= 0 && py < fb_height) {
                for (int col = 0; col < w; col++) {
                    framebuffer_set_pixel(x + col, py, color);
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
    // Current framebuffer (back buffer) is now ready to be sent
    uint8_t *prev_buffer = framebuffer_v[back_buffer_idx];
    
    // Toggle buffer index for the NEXT frame
    if (fb_format == PIXEL_FORMAT_RGB444) {
        back_buffer_idx = 1 - back_buffer_idx;
        // Wait for previous DMA to finish before starting new one
        framebuffer_wait_last_swap();
    }
    
    display_set_window(0, 0, fb_width - 1, fb_height - 1);
    display_start_write();
    dma_spi_transfer(prev_buffer, fb_size);
    dma_active = true;
}

void framebuffer_wait_last_swap(void) {
    if (dma_active) {
        dma_spi_wait();
        display_end_write();
        dma_active = false;
    }
}
