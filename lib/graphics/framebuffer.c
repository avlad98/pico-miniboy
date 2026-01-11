#include "framebuffer.h"
#include "display_driver.h"
#include "dma_spi.h"
#include <string.h>
#include <stdlib.h>

static uint8_t *framebuffer = NULL;
static uint16_t fb_width = 0;
static uint16_t fb_height = 0;
static uint32_t fb_size = 0;

bool framebuffer_init(uint16_t width, uint16_t height) {
    fb_width = width;
    fb_height = height;
    fb_size = width * height * 2;  // RGB565 = 2 bytes per pixel
    
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
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    uint32_t color32 = (lo << 24) | (hi << 16) | (lo << 8) | hi;
    
    uint32_t *ptr32 = (uint32_t*)framebuffer;
    int count = fb_size / 4;
    
    while (count--) {
        *ptr32++ = color32;
    }
}

void framebuffer_set_pixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= fb_width || y < 0 || y >= fb_height) {
        return;
    }
    
    int idx = (y * fb_width + x) * 2;
    framebuffer[idx] = color >> 8;
    framebuffer[idx + 1] = color & 0xFF;
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
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    int r2 = radius * radius;
    
    for (int y = -radius; y <= radius; y++) {
        int py = cy + y;
        if (py >= 0 && py < fb_height) {
            int x_extent = 0;
            while (x_extent * x_extent + y * y <= r2) x_extent++;
            
            int x_start = cx - x_extent;
            int x_end = cx + x_extent;
            
            if (x_start < 0) x_start = 0;
            if (x_end >= fb_width) x_end = fb_width - 1;
            
            for (int x = x_start; x <= x_end; x++) {
                int idx = (py * fb_width + x) * 2;
                framebuffer[idx] = hi;
                framebuffer[idx + 1] = lo;
            }
        }
    }
}

void framebuffer_swap(void) {
    // Set display window to full screen
    display_set_window(0, 0, fb_width - 1, fb_height - 1);
    
    // Start write mode
    display_start_write();
    
    // Transfer via DMA
    dma_spi_transfer(framebuffer, fb_size);
    dma_spi_wait();
    
    // End write mode
    display_end_write();
}
