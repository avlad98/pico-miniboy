#ifndef PROFILER_H
#define PROFILER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float cpu0_usage_percent;
    float cpu1_usage_percent;
    uint32_t ram_used_bytes;
    uint32_t ram_total_bytes;
    uint32_t flash_used_bytes;
    uint32_t flash_total_bytes;
    uint32_t fps;
} system_stats_t;

// Initialize the profiler
void profiler_init(void);

// Update stats (call once per frame or second)
void profiler_update(uint32_t frame_time_us);

// Get the latest snapshot
system_stats_t profiler_get_stats(void);

// Draw the stats overlay
void profiler_draw(void);

#endif
