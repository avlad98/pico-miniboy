#include "profiler.h"
#include "pico/stdlib.h"
#include "framebuffer.h"
#include "font.h"
#include <malloc.h>
#include <string.h>
#include <stdio.h>

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

void profiler_init(void) {
    memset(&current_stats, 0, sizeof(system_stats_t));
    current_stats.ram_total_bytes = TOTAL_RAM_BYTES;
    current_stats.flash_total_bytes = TOTAL_FLASH_BYTES;
    
    // Calculate static flash usage
    uint32_t flash_used = (uint32_t)&__flash_binary_end - (uint32_t)&__flash_binary_start;
    current_stats.flash_used_bytes = flash_used;
}

void profiler_update(uint32_t frame_time_us) {
    frame_accumulator++;
    time_accumulator += frame_time_us;
    
    // Update every 0.5 seconds for readability
    if (time_accumulator >= 500000) {
        // --- FPS ---
        current_stats.fps = (frame_accumulator * 1000000) / time_accumulator;
        
        // --- CPU 0 Usage ---
        // CPU0 Active = Total Time - Wait Time
        // Usage = (Active / Total) * 100
        uint32_t wait_us = framebuffer_get_last_wait_time();
        // Wait time is per-frame. We need average wait per frame ideally, 
        // but getting the *last* frame's wait is a decent instant snapshot.
        // Actually, let's use the Ratio of "Wait in this frame" vs "Frame Time".
        
        float active_ratio = 1.0f;
        if (frame_time_us > 0 && wait_us < frame_time_us) {
            active_ratio = (float)(frame_time_us - wait_us) / (float)frame_time_us;
        } else if (wait_us >= frame_time_us) {
            active_ratio = 0.0f; // All wait
        }
        current_stats.cpu0_usage_percent = active_ratio * 100.0f;
        
        // --- CPU 1 Usage ---
        // We accumulate Core 1 busy time over many frames? 
        // No, framebuffer.c accumulates it since last reset? 
        // Use the cumulative counter since last update.
        uint32_t c1_busy = framebuffer_get_core1_busy_us();
        float c1_ratio = (float)c1_busy / (float)time_accumulator;
        if (c1_ratio > 1.0f) c1_ratio = 1.0f;
        current_stats.cpu1_usage_percent = c1_ratio * 100.0f;
        
        // Reset Logic
        framebuffer_reset_profile_stats();
        frame_accumulator = 0;
        time_accumulator = 0;
        
        // --- RAM Usage ---
        struct mallinfo m = mallinfo();
        current_stats.ram_used_bytes = m.uordblks; // Used heap
        // Note: This doesn't include stack, but heap is the main dynamic part.
    }
}

system_stats_t profiler_get_stats(void) {
    return current_stats;
}

void profiler_draw(void) {
    // Draw Background Bar (Top of screen, 12px height)
    framebuffer_fill_rect(0, 0, 320, 12, 0x0000); 
    
    int y = 2; // Top padding
    
    // --- CPU Core 0 ---
    // "C0:XX"
    font_draw_char(5, y, 'C', 0xAAAA, 0x0000, &font_5x7); // Gray label
    font_draw_char(11, y, '0', 0xAAAA, 0x0000, &font_5x7);
    font_draw_number(23, y, (int)current_stats.cpu0_usage_percent, 0x07E0, 0x0000, &font_5x7);
    
    // --- CPU Core 1 ---
    // "C1:XX"
    font_draw_char(55, y, 'C', 0xAAAA, 0x0000, &font_5x7);
    font_draw_char(61, y, '1', 0xAAAA, 0x0000, &font_5x7);
    font_draw_number(73, y, (int)current_stats.cpu1_usage_percent, 0x07E0, 0x0000, &font_5x7);
    
    // --- FPS ---
    // "FPS:XXX"
    font_draw_char(110, y, 'F', 0xFFFF, 0x0000, &font_5x7);
    font_draw_char(116, y, 'P', 0xFFFF, 0x0000, &font_5x7);
    font_draw_char(122, y, 'S', 0xFFFF, 0x0000, &font_5x7);
    font_draw_number(140, y, current_stats.fps, 0xFFE0, 0x0000, &font_5x7); // Yellow FPS

    // --- RAM ---
    // "R:XXk"
    font_draw_char(180, y, 'R', 0xAAAA, 0x0000, &font_5x7);
    font_draw_char(186, y, 'A', 0xAAAA, 0x0000, &font_5x7);
    font_draw_char(192, y, 'M', 0xAAAA, 0x0000, &font_5x7);
    font_draw_number(210, y, current_stats.ram_used_bytes / 1024, 0x07FF, 0x0000, &font_5x7); // Cyan
    font_draw_char(230, y, 'k', 0xAAAA, 0x0000, &font_5x7);

    // Flash usually static, maybe skip to save space or move to end
    // "F:XXk"
    font_draw_char(250, y, 'F', 0xAAAA, 0x0000, &font_5x7);
    font_draw_char(256, y, 'L', 0xAAAA, 0x0000, &font_5x7);
    font_draw_number(274, y, current_stats.flash_used_bytes / 1024, 0xF800, 0x0000, &font_5x7); // Red
    font_draw_char(294, y, 'k', 0xAAAA, 0x0000, &font_5x7);
}
