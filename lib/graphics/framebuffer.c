#include "framebuffer.h"
#include "display_driver.h"
#include "dma_mem.h"
#include "render_service.h"
#include "surface.h"
#include <stdlib.h>
#include <string.h>

static surface_t surfaces[3]; // Max 3 buffers
static uint8_t *expansion_buffer = NULL;
static uint8_t back_buffer_idx = 0;
static uint8_t front_buffer_idx = 0; // Only relevant for >1 buffers
static uint8_t buffer_count = 2;     // Default
static uint16_t rgb332_to_rgb565[256];
static bool lut_initialized = false;
static bool swap_active = false; // DMA is busy

// Instrumentation
static volatile uint32_t last_wait_time_us = 0;

bool framebuffer_init(uint16_t width, uint16_t height,
                      display_pixel_format_t format, uint8_t count) {
  if (count > 3)
    count = 3;
  buffer_count = count; // 0 = Direct Mode
  back_buffer_idx = 0;
  front_buffer_idx = 0;

  uint32_t fb_size = 0;
  if (count > 0) {
    if (format == PIXEL_FORMAT_RGB565) {
      fb_size = width * height * 2;
    } else if (format == PIXEL_FORMAT_RGB444) {
      fb_size = (width * height * 3) / 2;
    } else { // RGB332
      fb_size = width * height;
      // Line buffers for streaming expansion (Ping-Pong: 2 lines)
      expansion_buffer = (uint8_t *)malloc(width * 2 * 2);
      if (expansion_buffer == NULL)
        return false;
    }
  }

  for (int i = 0; i < 3; i++) {
    if (i < count && count > 0) {
      surfaces[i].pixels = (uint8_t *)malloc(fb_size);
      if (surfaces[i].pixels == NULL)
        return false;
      memset(surfaces[i].pixels, 0, fb_size);
    } else {
      surfaces[i].pixels = NULL; // Direct mode or unused
    }
    surfaces[i].width = width;
    surfaces[i].height = height;
    surfaces[i].format = format;
    surfaces[i].size = fb_size;
  }

  if (!lut_initialized) {
    for (int i = 0; i < 256; i++) {
      uint8_t r3 = (i >> 5) & 0x07;
      uint8_t g3 = (i >> 2) & 0x07;
      uint8_t b2 = i & 0x03;
      uint16_t r5 = (r3 << 2) | (r3 >> 1);
      uint16_t g6 = (g3 << 3) | g3;
      uint16_t b5 = (b2 << 3) | (b2 << 1) | (b2 >> 1);
      uint16_t rgb565 = (r5 << 11) | (g6 << 5) | b5;
      rgb332_to_rgb565[i] = (rgb565 >> 8) | ((rgb565 & 0xFF) << 8);
    }
    lut_initialized = true;
  }

  dma_mem_init();
  render_service_init();

  return true;
}

surface_t *framebuffer_get_surface(void) { return &surfaces[back_buffer_idx]; }

void draw_clear(surface_t *surf, uint16_t color) {
  if (surf->pixels == NULL) {
    // Direct Mode Flood
    display_set_window(0, 0, surf->width - 1, surf->height - 1);
    display_start_bulk();
    display_push_pixels(color, (uint32_t)surf->width * surf->height);
    display_end_bulk();
    return;
  }

  // Note: This still uses the render_service for multicore clearing
  uint32_t total_bytes = surf->size;
  uint32_t half_bytes = total_bytes / 2;

  if (surf->format == PIXEL_FORMAT_RGB444) {
    half_bytes -= (half_bytes % 12);
  } else {
    half_bytes -= (half_bytes % 4);
  }

  uint32_t color32 = 0;
  if (surf->format == PIXEL_FORMAT_RGB565) {
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    color32 = (lo << 24) | (hi << 16) | (lo << 8) | hi;
  } else if (surf->format == PIXEL_FORMAT_RGB332) {
    uint8_t r3 = (color >> 13) & 0x07;
    uint8_t g3 = (color >> 8) & 0x07;
    uint8_t b2 = (color >> 3) & 0x03;
    uint8_t c8 = (r3 << 5) | (g3 << 2) | b2;
    color32 = c8 | (c8 << 8) | (c8 << 16) | (c8 << 24);
  }

  // Submit job to Core 1
  render_job_t job = {.type = RENDER_CMD_CLEAR,
                      .surface = surf,
                      .color16 = color,
                      .color32 = color32,
                      .start_offset = half_bytes,
                      .count = (total_bytes - half_bytes) / 4};
  render_service_submit(&job);

  // Core 0 Job
  if (surf->format == PIXEL_FORMAT_RGB444) {
    uint8_t r4 = ((color >> 11) & 0x1F) >> 1;
    uint8_t g4 = ((color >> 5) & 0x3F) >> 2;
    uint8_t b4 = (color & 0x1F) >> 1;
    uint8_t b0 = (r4 << 4) | g4;
    uint8_t b1 = (b4 << 4) | r4;
    uint8_t b2 = (g4 << 4) | b4;
    uint32_t w0 = b0 | (b1 << 8) | (b2 << 16) | (b0 << 24);
    uint32_t w1 = b1 | (b2 << 8) | (b0 << 16) | (b1 << 24);
    uint32_t w2 = b2 | (b0 << 8) | (b1 << 16) | (b2 << 24);

    uint32_t *ptr32 = (uint32_t *)surf->pixels;
    uint32_t chunk_count = half_bytes / 12;
    for (uint32_t i = 0; i < chunk_count; i++) {
      *ptr32++ = w0;
      *ptr32++ = w1;
      *ptr32++ = w2;
    }
  } else {
    dma_mem_fill32((uint32_t *)surf->pixels, color32, half_bytes / 4);
  }

  render_service_wait();
  dma_mem_wait();
}

void draw_pixel(surface_t *surf, int x, int y, uint16_t color) {
  if (x < 0 || x >= surf->width || y < 0 || y >= surf->height)
    return;

  if (surf->pixels == NULL) {
    // Direct Mode
    display_set_window(x, y, x, y);
    display_start_bulk();
    display_push_pixels(color, 1);
    display_end_bulk();
    return;
  }

  if (surf->format == PIXEL_FORMAT_RGB565) {
    int idx = (y * surf->width + x) * 2;
    surf->pixels[idx] = color >> 8;
    surf->pixels[idx + 1] = color & 0xFF;
  } else if (surf->format == PIXEL_FORMAT_RGB444) {
    int pixel_idx = y * surf->width + x;
    int byte_idx = (pixel_idx / 2) * 3;
    uint8_t r4 = ((color >> 11) & 0x1F) >> 1;
    uint8_t g4 = ((color >> 5) & 0x3F) >> 2;
    uint8_t b4 = (color & 0x1F) >> 1;
    if (pixel_idx % 2 == 0) {
      surf->pixels[byte_idx] = (r4 << 4) | g4;
      surf->pixels[byte_idx + 1] =
          (b4 << 4) | (surf->pixels[byte_idx + 1] & 0x0F);
    } else {
      surf->pixels[byte_idx + 1] = (surf->pixels[byte_idx + 1] & 0xF0) | r4;
      surf->pixels[byte_idx + 2] = (g4 << 4) | b4;
    }
  } else { // RGB332
    int idx = y * surf->width + x;
    uint8_t r3 = (color >> 13) & 0x07;
    uint8_t g3 = (color >> 8) & 0x07;
    uint8_t b2 = (color >> 3) & 0x03;
    surf->pixels[idx] = (r3 << 5) | (g3 << 2) | b2;
  }
}

void draw_rect(surface_t *surf, int x, int y, int w, int h, uint16_t color) {
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > surf->width)
    w = surf->width - x;
  if (y + h > surf->height)
    h = surf->height - y;
  if (w <= 0 || h <= 0)
    return;

  if (surf->pixels == NULL) {
    // Direct Mode
    display_set_window(x, y, x + w - 1, y + h - 1);
    display_start_bulk();
    display_push_pixels(color, (uint32_t)w * h);
    display_end_bulk();
    return;
  }

  if (surf->format == PIXEL_FORMAT_RGB444) {
    uint8_t r4 = ((color >> 11) & 0x1F) >> 1;
    uint8_t g4 = ((color >> 5) & 0x3F) >> 2;
    uint8_t b4 = (color & 0x1F) >> 1;
    uint8_t b0 = (r4 << 4) | g4;
    uint8_t b1 = (b4 << 4) | r4;
    uint8_t b2 = (g4 << 4) | b4;
    uint32_t w0 = b0 | (b1 << 8) | (b2 << 16) | (b0 << 24);
    uint32_t w1 = b1 | (b2 << 8) | (b0 << 16) | (b1 << 24);
    uint32_t w2 = b2 | (b0 << 8) | (b1 << 16) | (b2 << 24);

    for (int row = 0; row < h; row++) {
      int py = y + row;
      int start_pixel = py * surf->width + x;
      if ((start_pixel % 8 == 0) && (w % 8 == 0)) {
        uint32_t *ptr32 = (uint32_t *)&surf->pixels[(start_pixel / 2) * 3];
        int words = (w / 8) * 3;
        for (int i = 0; i < words; i += 3) {
          ptr32[i] = w0;
          ptr32[i + 1] = w1;
          ptr32[i + 2] = w2;
        }
      } else {
        for (int px = x; px < x + w; px++)
          draw_pixel(surf, px, py, color);
      }
    }
  } else {
    for (int row = 0; row < h; row++) {
      for (int col = 0; col < w; col++) {
        draw_pixel(surf, x + col, y + row, color);
      }
    }
  }
}

void draw_circle(surface_t *surf, int cx, int cy, int radius, uint16_t color) {
  int x = radius;
  int y = 0;
  int err = 0;
  while (x >= y) {
    draw_rect(surf, cx - x, cy + y, 2 * x + 1, 1, color);
    draw_rect(surf, cx - x, cy - y, 2 * x + 1, 1, color);
    draw_rect(surf, cx - y, cy + x, 2 * y + 1, 1, color);
    draw_rect(surf, cx - y, cy - x, 2 * y + 1, 1, color);
    y++;
    err += 1 + 2 * y;
    if (2 * (err - x) + 1 > 0) {
      x--;
      err += 1 - 2 * x;
    }
  }
}

void framebuffer_clear(uint16_t color) {
  draw_clear(framebuffer_get_surface(), color);
}

void framebuffer_fill_circle(int cx, int cy, int radius, uint16_t color) {
  draw_circle(framebuffer_get_surface(), cx, cy, radius, color);
}

void framebuffer_wait_last_swap(void) {
  if (swap_active) {
    uint32_t start = time_us_32();
    display_end_bulk(); // Wait for DMA
    last_wait_time_us += (time_us_32() - start);
    swap_active = false;
  }
}

void framebuffer_swap_async(void) {
  surface_t *surf = &surfaces[back_buffer_idx];
  uint8_t *send_buffer = surf->pixels;
  uint32_t send_size = surf->size;

  if (surf->format == PIXEL_FORMAT_RGB332) {
    // RGB332 Streaming Mode (Line-by-Line)
    // Minimizes RAM usage to enable Double/Triple buffering.
    framebuffer_wait_last_swap(); // Ensure previous frame is done
    
    uint16_t *line_buf = (uint16_t *)expansion_buffer;
    
    for (int y = 0; y < surf->height; y++) {
        // 1. Convert Line
        uint8_t *src = surf->pixels + (y * surf->width);
        for (int x = 0; x < surf->width; x++) {
            line_buf[x] = rgb332_to_rgb565[src[x]];
        }
        
        // 2. Send Line Sync
        display_set_window(0, y, surf->width - 1, y);
        display_start_bulk();
        display_send_buffer((uint8_t*)line_buf, surf->width * 2);
        display_end_bulk(); // Waits for DMA
    }
    
    // Update Buffer Index manually as we sent the frame line-by-line
    
    if (buffer_count == 2) {
        back_buffer_idx = 1 - back_buffer_idx;
    } else if (buffer_count == 3) {
        back_buffer_idx = (back_buffer_idx + 1) % 3;
    }
    
    swap_active = false; // We finished synchronously.
    return;
  }

  // --- Swap Logic ---

  if (buffer_count == 1) {
    // Single Buffer: MUST wait for previous frame to finish before sending
    // again
    framebuffer_wait_last_swap();
    // Now trigger send
    display_set_window(0, 0, surf->width - 1, surf->height - 1);
    display_start_bulk();
    display_send_buffer(send_buffer, send_size);
    swap_active = true;

    // Tearing artifacts are expected in Single Buffer mode.
  } else if (buffer_count == 2) {
    // Double Buffer: Standard ping-pong
    // 1. Wait for PREVIOUS DMA to finish
    framebuffer_wait_last_swap();

    // 2. Start DMA on CURRENT back buffer
    display_set_window(0, 0, surf->width - 1, surf->height - 1);
    display_start_bulk();
    display_send_buffer(send_buffer, send_size);
    swap_active = true;

    // 3. Flip index
    back_buffer_idx = 1 - back_buffer_idx;
  } else if (buffer_count == 3) {
    // Triple Buffer
    framebuffer_wait_last_swap();

    display_set_window(0, 0, surf->width - 1, surf->height - 1);
    display_start_bulk();
    display_send_buffer(send_buffer, send_size);
    swap_active = true;

    back_buffer_idx = (back_buffer_idx + 1) % 3;
  }
}

uint32_t framebuffer_get_last_wait_time(void) { return last_wait_time_us; }
uint8_t framebuffer_get_buffer_count(void) { return buffer_count; }
void framebuffer_reset_profile_stats(void) {
  render_service_reset_stats();
  last_wait_time_us = 0;
}
