#include "framebuffer.h"
#include "display_driver.h"
#include "dma_mem.h"
#include "pico/multicore.h"
#include <stdlib.h>
#include <string.h>

static uint8_t *framebuffer_v[2] = {NULL, NULL};
static uint8_t *expansion_buffer = NULL;
static uint8_t back_buffer_idx = 0;
static uint16_t fb_width = 0;
static uint16_t fb_height = 0;
static uint32_t fb_size = 0;
static display_pixel_format_t fb_format;
static uint16_t rgb332_to_rgb565[256];
static bool lut_initialized = false;
static bool dma_active = false;

#define framebuffer framebuffer_v[back_buffer_idx]

// --- Multicore Rendering Engine ---
typedef enum { CMD_IDLE, CMD_CLEAR, CMD_RECT } cmd_type_t;

typedef struct {
  cmd_type_t type;
  uint32_t color32; // Pre-calculated for RGB444/332 block writes
  uint16_t color16;
  uint32_t start_offset; // For CLEAR (byte offset)
  uint32_t count;        // For CLEAR (32-bit word count)
  int x, y, w, h;        // For RECT
} render_job_t;

static volatile render_job_t core1_job;

// ... helper for color packing ...
uint32_t pack_rgb444_block(uint16_t color) {
  uint8_t r4 = ((color >> 11) & 0x1F) >> 1;
  uint8_t g4 = ((color >> 5) & 0x3F) >> 2;
  uint8_t b4 = (color & 0x1F) >> 1;
  uint8_t b0 = (r4 << 4) | g4;
  uint8_t b1 = (b4 << 4) | r4;
  uint8_t b2 = (g4 << 4) | b4;
  return b0 | (b1 << 8) | (b2 << 16) | (b0 << 24);
}

// Core 1 Performance Tracking
volatile uint32_t core1_busy_us = 0;
static uint32_t c1_start_time = 0;

// Core 1 Worker Loop
void core1_render_entry() {
  while (1) {
    // Wait for a signal from Core 0
    uint32_t cmd = multicore_fifo_pop_blocking();

    c1_start_time = time_us_32();

    uint8_t *fb_ptr = framebuffer_v[back_buffer_idx];

    if (cmd == CMD_CLEAR) {
      if (fb_format == PIXEL_FORMAT_RGB444) {
        // RGB444: 3 bytes per 2 pixels.
        uint32_t w0, w1, w2;

        uint8_t r4 = ((core1_job.color16 >> 11) & 0x1F) >> 1;
        uint8_t g4 = ((core1_job.color16 >> 5) & 0x3F) >> 2;
        uint8_t b4 = (core1_job.color16 & 0x1F) >> 1;
        uint8_t b0 = (r4 << 4) | g4;
        uint8_t b1 = (b4 << 4) | r4;
        uint8_t b2 = (g4 << 4) | b4;
        w0 = b0 | (b1 << 8) | (b2 << 16) | (b0 << 24);
        w1 = b1 | (b2 << 8) | (b0 << 16) | (b1 << 24);
        w2 = b2 | (b0 << 8) | (b1 << 16) | (b2 << 24);

        uint32_t *ptr32 = (uint32_t *)(fb_ptr + core1_job.start_offset);
        uint32_t chunk_count = (core1_job.count * 4) / 12;
        for (uint32_t i = 0; i < chunk_count; i++) {
          *ptr32++ = w0;
          *ptr32++ = w1;
          *ptr32++ = w2;
        }
      } else {
        // RGB565 / RGB332 (Aligned 32-bit works perfectly here)
        uint32_t *ptr = (uint32_t *)(fb_ptr + core1_job.start_offset);
        uint32_t *end = ptr + core1_job.count;
        uint32_t val = core1_job.color32;
        while (ptr < end)
          *ptr++ = val;
      }
    }

    // Accumulate busy time
    core1_busy_us += (time_us_32() - c1_start_time);

    multicore_fifo_push_blocking(1);
  }
}

bool framebuffer_init(uint16_t width, uint16_t height,
                      display_pixel_format_t format) {
  fb_width = width;
  fb_height = height;
  fb_format = format;

  if (format == PIXEL_FORMAT_RGB565) {
    fb_size = width * height * 2;
  } else if (format == PIXEL_FORMAT_RGB444) {
    fb_size = (width * height * 3) / 2;
  } else { // RGB332
    fb_size = width * height;
    expansion_buffer = (uint8_t *)malloc(width * height * 2);
    if (expansion_buffer == NULL)
      return false;
  }

  framebuffer_v[0] = (uint8_t *)malloc(fb_size);
  if (format == PIXEL_FORMAT_RGB444 || format == PIXEL_FORMAT_RGB332) {
    framebuffer_v[1] = (uint8_t *)malloc(fb_size);
  } else {
    framebuffer_v[1] = framebuffer_v[0];
  }

  if (framebuffer_v[0] == NULL || framebuffer_v[1] == NULL) {
    return false;
  }

  memset(framebuffer_v[0], 0, fb_size);
  memset(framebuffer_v[1], 0, fb_size);

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

  // Launch Render Slave
  multicore_launch_core1(core1_render_entry);

  return true;
}

void framebuffer_clear(uint16_t color) {
  framebuffer_wait_clear();

  // We split by BYTES for simplicity in calculating offsets
  // RGB565: Size = W*H*2
  // RGB444: Size = W*H*1.5
  uint32_t total_bytes = fb_size;
  uint32_t half_bytes = total_bytes / 2;

  // Align half_bytes to 12 (3 words) for RGB444 safety
  if (fb_format == PIXEL_FORMAT_RGB444) {
    half_bytes -= (half_bytes % 12);
  } else {
    half_bytes -= (half_bytes % 4);
  }

  uint32_t color32 = 0;

  if (fb_format == PIXEL_FORMAT_RGB565) {
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    color32 = (lo << 24) | (hi << 16) | (lo << 8) | hi;
  } else if (fb_format == PIXEL_FORMAT_RGB332) {
    uint8_t r3 = (color >> 13) & 0x07;
    uint8_t g3 = (color >> 8) & 0x07;
    uint8_t b2 = (color >> 3) & 0x03;
    uint8_t c8 = (r3 << 5) | (g3 << 2) | b2;
    color32 = c8 | (c8 << 8) | (c8 << 16) | (c8 << 24);
  }

  // Prepare Core 1 Job (Bottom Half)
  core1_job.type = CMD_CLEAR;
  core1_job.color16 = color; // For RGB444 re-calc
  core1_job.color32 = color32;
  core1_job.start_offset = half_bytes;
  core1_job.count = (total_bytes - half_bytes) / 4;

  multicore_fifo_push_blocking(CMD_CLEAR);

  // Core 0 Job (Top Half)
  if (fb_format == PIXEL_FORMAT_RGB444) {
    // Use CPU loop for RGB444 to ensure pattern correctness
    uint8_t r4 = ((color >> 11) & 0x1F) >> 1;
    uint8_t g4 = ((color >> 5) & 0x3F) >> 2;
    uint8_t b4 = (color & 0x1F) >> 1;
    uint8_t b0 = (r4 << 4) | g4;
    uint8_t b1 = (b4 << 4) | r4;
    uint8_t b2 = (g4 << 4) | b4;
    uint32_t w0 = b0 | (b1 << 8) | (b2 << 16) | (b0 << 24);
    uint32_t w1 = b1 | (b2 << 8) | (b0 << 16) | (b1 << 24);
    uint32_t w2 = b2 | (b0 << 8) | (b1 << 16) | (b2 << 24);

    uint32_t *ptr32 = (uint32_t *)framebuffer;
    uint32_t chunk_count = half_bytes / 12; // 3 words = 12 bytes
    for (uint32_t i = 0; i < chunk_count; i++) {
      *ptr32++ = w0;
      *ptr32++ = w1;
      *ptr32++ = w2;
    }
  } else {
    // Use DMA for 32-bit aligned modes (RGB565/332)
    dma_mem_fill32((uint32_t *)framebuffer, color32, half_bytes / 4);
  }

  multicore_fifo_pop_blocking();

  // Note: Core 0 DMA might still be running for RGB565/332.
  // Wait for it to finish to prevent CPU/DMA races on the top half.
  dma_mem_wait();
}

// Instrumentation for CPU Usage
static volatile uint32_t last_wait_time_us = 0;

uint32_t framebuffer_get_last_wait_time(void) { return last_wait_time_us; }

uint32_t framebuffer_get_core1_busy_us(void) { return core1_busy_us; }

void framebuffer_reset_profile_stats(void) {
  core1_busy_us = 0;
  last_wait_time_us = 0; // Reset for new frame measurement
}

void framebuffer_wait_clear(void) {
  uint32_t start = time_us_32();
  dma_mem_wait();
  last_wait_time_us += (time_us_32() - start);
}

void framebuffer_set_pixel(int x, int y, uint16_t color) {
  framebuffer_wait_clear();
  if (x < 0 || x >= fb_width || y < 0 || y >= fb_height)
    return;

  if (fb_format == PIXEL_FORMAT_RGB565) {
    int idx = (y * fb_width + x) * 2;
    framebuffer[idx] = color >> 8;
    framebuffer[idx + 1] = color & 0xFF;
  } else if (fb_format == PIXEL_FORMAT_RGB444) {
    int pixel_idx = y * fb_width + x;
    int byte_idx = (pixel_idx / 2) * 3;
    uint8_t r4 = ((color >> 11) & 0x1F) >> 1;
    uint8_t g4 = ((color >> 5) & 0x3F) >> 2;
    uint8_t b4 = (color & 0x1F) >> 1;

    if (pixel_idx % 2 == 0) {
      framebuffer[byte_idx] = (r4 << 4) | g4;
      framebuffer[byte_idx + 1] =
          (b4 << 4) | (framebuffer[byte_idx + 1] & 0x0F);
    } else {
      framebuffer[byte_idx + 1] = (framebuffer[byte_idx + 1] & 0xF0) | r4;
      framebuffer[byte_idx + 2] = (g4 << 4) | b4;
    }
  } else { // RGB332
    int idx = y * fb_width + x;
    uint8_t r3 = (color >> 13) & 0x07;
    uint8_t g3 = (color >> 8) & 0x07;
    uint8_t b2 = (color >> 3) & 0x03;
    framebuffer[idx] = (r3 << 5) | (g3 << 2) | b2;
  }
}

void framebuffer_fill_rect(int x, int y, int w, int h, uint16_t color) {
  framebuffer_wait_clear();
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > fb_width)
    w = fb_width - x;
  if (y + h > fb_height)
    h = fb_height - y;
  if (w <= 0 || h <= 0)
    return;

  if (fb_format == PIXEL_FORMAT_RGB444) {
    uint32_t w0, w1, w2;
    uint32_t color32 = pack_rgb444_block(color);
    // unpack back to 3 bytes if needed or just re-calculate
    uint8_t r4 = ((color >> 11) & 0x1F) >> 1;
    uint8_t g4 = ((color >> 5) & 0x3F) >> 2;
    uint8_t b4 = (color & 0x1F) >> 1;
    uint8_t b0 = (r4 << 4) | g4;
    uint8_t b1 = (b4 << 4) | r4;
    uint8_t b2 = (g4 << 4) | b4;
    w0 = b0 | (b1 << 8) | (b2 << 16) | (b0 << 24);
    w1 = b1 | (b2 << 8) | (b0 << 16) | (b1 << 24);
    w2 = b2 | (b0 << 8) | (b1 << 16) | (b2 << 24);

    for (int row = 0; row < h; row++) {
      int py = y + row;
      int start_pixel = py * fb_width + x;
      if ((start_pixel % 8 == 0) && (w % 8 == 0)) {
        uint32_t *ptr32 = (uint32_t *)&framebuffer[(start_pixel / 2) * 3];
        int words = (w / 8) * 3;
        for (int i = 0; i < words; i += 3) {
          ptr32[i] = w0;
          ptr32[i + 1] = w1;
          ptr32[i + 2] = w2;
        }
      } else {
        for (int px = x; px < x + w; px++)
          framebuffer_set_pixel(px, py, color);
      }
    }
  } else {
    for (int row = 0; row < h; row++) {
      for (int col = 0; col < w; col++) {
        framebuffer_set_pixel(x + col, y + row, color);
      }
    }
  }
}

void framebuffer_fill_circle(int cx, int cy, int radius, uint16_t color) {
  int x = radius;
  int y = 0;
  int err = 0;
  while (x >= y) {
    framebuffer_fill_rect(cx - x, cy + y, 2 * x + 1, 1, color);
    framebuffer_fill_rect(cx - x, cy - y, 2 * x + 1, 1, color);
    framebuffer_fill_rect(cx - y, cy + x, 2 * y + 1, 1, color);
    framebuffer_fill_rect(cx - y, cy - x, 2 * y + 1, 1, color);
    y++;
    err += 1 + 2 * y;
    if (2 * (err - x) + 1 > 0) {
      x--;
      err += 1 - 2 * x;
    }
  }
}

void framebuffer_wait_last_swap(void) {
  if (dma_active) {
    uint32_t start = time_us_32();
    display_end_bulk();
    last_wait_time_us += (time_us_32() - start);

    dma_active = false;
  }
}

void framebuffer_swap_async(void) {
  framebuffer_wait_clear();
  uint8_t *send_buffer = framebuffer_v[back_buffer_idx];
  uint32_t send_size = fb_size;

  if (fb_format == PIXEL_FORMAT_RGB332) {
    uint16_t *dst = (uint16_t *)expansion_buffer;
    for (uint32_t i = 0; i < fb_width * fb_height; i++) {
      dst[i] = rgb332_to_rgb565[send_buffer[i]];
    }
    send_buffer = expansion_buffer;
    send_size = fb_width * fb_height * 2;
  }

  if (fb_format == PIXEL_FORMAT_RGB444 || fb_format == PIXEL_FORMAT_RGB332) {
    back_buffer_idx = 1 - back_buffer_idx;
    framebuffer_wait_last_swap();
  }

  display_set_window(0, 0, fb_width - 1, fb_height - 1);
  display_start_bulk();
  display_send_buffer(send_buffer, send_size);
  dma_active = true;
}
