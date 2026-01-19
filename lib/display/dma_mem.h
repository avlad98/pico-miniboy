#ifndef DMA_MEM_H
#define DMA_MEM_H

#include "pico/stdlib.h"

// Initialize memory DMA channel
void dma_mem_init(void);

// Fill memory with a 32-bit value asynchronously
void dma_mem_fill32(uint32_t *dest, uint32_t value, uint32_t count);

// Check if memory DMA is busy
bool dma_mem_is_busy(void);

// Wait for memory DMA to complete
void dma_mem_wait(void);

#endif
