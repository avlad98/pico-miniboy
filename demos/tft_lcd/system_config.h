#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include "pico/stdlib.h"

// Performance profiles with expected FPS
#define PERFORMANCE_STABLE     { .cpu_mhz = 270, .spi_hz_init = 10000000, .spi_hz_fast = 67500000 }  // 46-47 FPS (rock solid)
#define PERFORMANCE_COLD_BOOST { .cpu_mhz = 280, .spi_hz_init = 10000000, .spi_hz_fast = 70000000 }  // 47-51 FPS (glitchy, temp sensitive)
#define PERFORMANCE_UNSTABLE   { .cpu_mhz = 300, .spi_hz_init = 10000000, .spi_hz_fast = 75000000 }  // USB breaks

typedef struct {
    uint32_t cpu_mhz;
    uint32_t spi_hz_init;   // Slow for display init
    uint32_t spi_hz_fast;   // Fast for framebuffer
} system_config_t;

// Initialize system clocks and voltage
void system_init(const system_config_t *config);

// Get actual achieved frequencies
uint32_t system_get_cpu_hz(void);
uint32_t system_get_spi_hz(void);

// Set actual SPI speed (called by display driver)
void system_set_actual_spi_hz(uint32_t hz);

#endif
