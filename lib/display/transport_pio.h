#ifndef TRANSPORT_PIO_H
#define TRANSPORT_PIO_H

#include "display_transport.h"
#include "hardware/dma.h"
#include "hardware/pio.h"

typedef struct {
  PIO pio;
  uint sm;
  uint pin_sck;
  uint pin_mosi;
  uint pin_cs;
  uint pin_dc;
  uint dma_chan;
  uint pio_offset;
  float div_init;
  float div_fast;
} transport_pio_config_t;

display_transport_t *transport_pio_create(const transport_pio_config_t *config);

#endif
