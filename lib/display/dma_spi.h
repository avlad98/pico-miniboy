#ifndef DMA_SPI_H
#define DMA_SPI_H

#include "pico/stdlib.h"
#include "hardware/spi.h"

// Initialize DMA for SPI transfers
void dma_spi_init(spi_inst_t *spi);

// Transfer data via DMA (non-blocking)
void dma_spi_transfer(const uint8_t *data, uint32_t len);

// Wait for current transfer to complete
void dma_spi_wait(void);
bool dma_spi_is_busy(void);

#endif
