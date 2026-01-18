#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include "pico/stdlib.h"

// Performance profiles with expected FPS
#define PERFORMANCE_STABLE      { .cpu_mhz = 240, .spi_hz_init = 10000000, .spi_hz_fast = 60000000 }  // 42 FPS (Standard)
#define PERFORMANCE_BALANCED    { .cpu_mhz = 270, .spi_hz_init = 10000000, .spi_hz_fast = 67500000 }  // 47 FPS (Rock solid)
#define PERFORMANCE_TURBO_MAX   { .cpu_mhz = 160, .spi_hz_init = 10000000, .spi_hz_fast = 80000000 }  // 58-62 FPS (Sweet spot for many ILI9341)
#define PERFORMANCE_LUDICROUS   { .cpu_mhz = 200, .spi_hz_init = 10000000, .spi_hz_fast = 100000000 } // ~64-65 FPS (100MHz SPI via Div 2)

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
