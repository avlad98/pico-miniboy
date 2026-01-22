#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include "pico/stdlib.h"

// Performance profiles with expected FPS
#define PERFORMANCE_STABLE                                                     \
  {.cpu_mhz = 240, .spi_hz_init = 10000000, .spi_hz_fast = 60000000}           \
  // ~65 FPS (Standard)
#define PERFORMANCE_BALANCED                                                   \
  {.cpu_mhz = 270, .spi_hz_init = 10000000, .spi_hz_fast = 67500000}           \
  // ~74 FPS (Rock solid)
#define PERFORMANCE_TURBO                                                      \
  {.cpu_mhz = 160, .spi_hz_init = 10000000, .spi_hz_fast = 80000000} // ~87 FPS
#define PERFORMANCE_HIGH                                                       \
  {.cpu_mhz = 190, .spi_hz_init = 10000000, .spi_hz_fast = 95000000}           \
  // ~103 FPS (Stable record)
#define PERFORMANCE_MAX                                                        \
  {.cpu_mhz = 220, .spi_hz_init = 10000000, .spi_hz_fast = 110000000}          \
  // ~119 FPS (Needs <5cm wires)
#define PERFORMANCE_EXTREME                                                    \
  {.cpu_mhz = 266, .spi_hz_init = 10000000, .spi_hz_fast = 133000000}          \
  // ~144 FPS (Theoretical)

typedef struct {
  uint32_t cpu_mhz;
  uint32_t spi_hz_init; // Slow for display init
  uint32_t spi_hz_fast; // Fast for framebuffer
} system_config_t;

extern const system_config_t system_profiles[];
extern const int system_profile_count;

// Initialize system clocks and voltage
void system_init(const system_config_t *config);

// Get actual achieved frequencies
uint32_t system_get_cpu_hz(void);
uint32_t system_get_spi_hz(void);

// Set actual SPI speed (called by display driver)
void system_set_actual_spi_hz(uint32_t hz);

#endif
