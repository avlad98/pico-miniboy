#ifndef DISPLAY_TRANSPORT_H
#define DISPLAY_TRANSPORT_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Display Transport Interface
 * Abstraction layer for sending data to different display panels
 * via PIO, Hardware SPI, or other protocols.
 */

typedef struct display_transport {
  // Initializer
  void (*init)(struct display_transport *self, uint32_t speed_init_hz,
               uint32_t speed_fast_hz);

  // Set speed mode (e.g. slow for commands, fast for data)
  void (*set_speed)(struct display_transport *self, bool fast);

  // Low level byte-by-byte transfer (blocking)
  void (*send_cmd)(struct display_transport *self, uint8_t cmd);
  void (*send_data8)(struct display_transport *self, uint8_t data);

  // High performance bulk transfer (can be async)
  void (*send_buffer)(struct display_transport *self, const uint8_t *data,
                      uint32_t len);

  // Synchronization
  void (*wait)(struct display_transport *self);
  bool (*is_busy)(struct display_transport *self);

  // Opaque pointer for internal state
  void *priv;
} display_transport_t;

#endif
