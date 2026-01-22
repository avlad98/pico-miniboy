#include "profiler.h"
#include "font.h"
#include "framebuffer.h"
#include "render_service.h"
#include <malloc.h>
#include <string.h>

// Linker symbols for Flash usage
extern char __flash_binary_start;
extern char __flash_binary_end;

// RAM Total: RP2040 has 264KB SRAM
#define TOTAL_RAM_BYTES (264 * 1024)
// Flash Total: Typically 2MB (but depends on board)
#define TOTAL_FLASH_BYTES (2 * 1024 * 1024)

static system_stats_t current_stats;
static uint32_t frame_accumulator = 0;
static uint32_t time_accumulator = 0;

#include "system_config.h"

void profiler_init(void) {
  memset(&current_stats, 0, sizeof(system_stats_t));
  current_stats.ram_total_bytes = TOTAL_RAM_BYTES;
  current_stats.flash_total_bytes = TOTAL_FLASH_BYTES;

  // Initial display info
  surface_t *surf = framebuffer_get_surface();
  if (surf) {
    current_stats.width = surf->width;
    current_stats.height = surf->height;
    if (surf->format == PIXEL_FORMAT_RGB565)
      current_stats.pixel_format = "RGB565";
    else if (surf->format == PIXEL_FORMAT_RGB444)
      current_stats.pixel_format = "RGB444";
    else
      current_stats.pixel_format = "RGB332";
  }

  // Calculate static flash usage
  uint32_t flash_used =
      (uint32_t)&__flash_binary_end - (uint32_t)&__flash_binary_start;
  current_stats.flash_used_bytes = flash_used;
}

void profiler_update(uint32_t frame_time_us) {
  frame_accumulator++;
  time_accumulator += frame_time_us;

  // Update every 0.5 seconds for readability
  if (time_accumulator >= 500000) {
    // --- FPS ---
    current_stats.fps = (frame_accumulator * 1000000) / time_accumulator;

    // --- System Info ---
    current_stats.cpu_hz = system_get_cpu_hz();
    current_stats.spi_hz = system_get_spi_hz();

    // Update display info in case it changed
    surface_t *surf = framebuffer_get_surface();
    if (surf) {
      current_stats.width = surf->width;
      current_stats.height = surf->height;
      if (surf->format == PIXEL_FORMAT_RGB565)
        current_stats.pixel_format = "RGB565";
      else if (surf->format == PIXEL_FORMAT_RGB444)
        current_stats.pixel_format = "RGB444";
      else
        current_stats.pixel_format = "RGB332";
    }

    // --- CPU 0 Usage ---
    uint32_t wait_us = framebuffer_get_last_wait_time();
    float active_ratio = 1.0f;
    if (frame_time_us > 0 && wait_us < frame_time_us) {
      active_ratio = (float)(frame_time_us - wait_us) / (float)frame_time_us;
    } else if (wait_us >= frame_time_us) {
      active_ratio = 0.0f; // All wait
    }
    current_stats.cpu0_usage_percent = active_ratio * 100.0f;

    // --- CPU 1 Usage ---
    uint32_t c1_busy = render_service_get_busy_us();
    float c1_ratio = (float)c1_busy / (float)time_accumulator;
    if (c1_ratio > 1.0f)
      c1_ratio = 1.0f;
    current_stats.cpu1_usage_percent = c1_ratio * 100.0f;

    // Reset Logic
    framebuffer_reset_profile_stats();
    frame_accumulator = 0;
    time_accumulator = 0;

    // --- RAM Usage ---
    struct mallinfo m = mallinfo();
    current_stats.ram_used_bytes = m.uordblks; // Used heap
  }
}

system_stats_t profiler_get_stats(void) { return current_stats; }

void profiler_draw(void) {
  surface_t *surf = framebuffer_get_surface();
  // Draw Background Bar (Top of screen, 24px height)
  draw_rect(surf, 0, 0, 320, 24, 0x0000);

  int y1 = 2;
  int y2 = 13;

  // --- Line 1: Performance ---
  font_draw_char(surf, 5, y1, 'C', 0xAAAA, 0x0000, &font_5x7);
  font_draw_char(surf, 11, y1, '0', 0xAAAA, 0x0000, &font_5x7);
  font_draw_number(surf, 23, y1, (int)current_stats.cpu0_usage_percent, 0x07E0,
                   0x0000, &font_5x7);

  font_draw_char(surf, 55, y1, 'C', 0xAAAA, 0x0000, &font_5x7);
  font_draw_char(surf, 61, y1, '1', 0xAAAA, 0x0000, &font_5x7);
  font_draw_number(surf, 73, y1, (int)current_stats.cpu1_usage_percent, 0x07E0,
                   0x0000, &font_5x7);

  font_draw_string(surf, 110, y1, "FPS:", 0xFFFF, 0x0000, &font_5x7);
  font_draw_number(surf, 135, y1, current_stats.fps, 0xFFE0, 0x0000, &font_5x7);

  font_draw_string(surf, 180, y1, "RAM:", 0xAAAA, 0x0000, &font_5x7);
  font_draw_number(surf, 205, y1, current_stats.ram_used_bytes / 1024, 0x07FF,
                   0x0000, &font_5x7);
  font_draw_char(surf, 225, y1, 'k', 0xAAAA, 0x0000, &font_5x7);

  font_draw_string(surf, 250, y1, "FLH:", 0xAAAA, 0x0000, &font_5x7);
  font_draw_number(surf, 275, y1, current_stats.flash_used_bytes / 1024, 0xF800,
                   0x0000, &font_5x7);
  font_draw_char(surf, 295, y1, 'k', 0xAAAA, 0x0000, &font_5x7);

  // --- Line 2: Config ---
  font_draw_string(surf, 5, y2, "CPU:", 0xAAAA, 0x0000, &font_5x7);
  font_draw_number(surf, 30, y2, current_stats.cpu_hz / 1000000, 0xFFFF, 0x0000,
                   &font_5x7);
  font_draw_string(surf, 50, y2, "MHz", 0xAAAA, 0x0000, &font_5x7);

  font_draw_string(surf, 80, y2, "SPI:", 0xAAAA, 0x0000, &font_5x7);
  font_draw_number(surf, 105, y2, current_stats.spi_hz / 1000000, 0xFFFF,
                   0x0000, &font_5x7);
  font_draw_string(surf, 125, y2, "MHz", 0xAAAA, 0x0000, &font_5x7);

  font_draw_string(surf, 160, y2, "MODE:", 0xAAAA, 0x0000, &font_5x7);
  if (current_stats.pixel_format)
    font_draw_string(surf, 195, y2, current_stats.pixel_format, 0xFBC0, 0x0000,
                     &font_5x7);

  font_draw_string(surf, 245, y2, "RES:", 0xAAAA, 0x0000, &font_5x7);
  font_draw_number(surf, 270, y2, current_stats.width, 0xFFFF, 0x0000,
                   &font_5x7);
  font_draw_char(surf, 290, y2, 'x', 0xAAAA, 0x0000, &font_5x7);
  font_draw_number(surf, 297, y2, current_stats.height, 0xFFFF, 0x0000,
                   &font_5x7);
}
