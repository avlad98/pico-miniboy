#include "dma_mem.h"
#include "hardware/dma.h"

static int dma_mem_channel = -1;
static volatile uint32_t fill_value __attribute__((aligned(4))) = 0;

void dma_mem_init(void) {
    if (dma_mem_channel != -1) return;
    
    dma_mem_channel = dma_claim_unused_channel(true);
    
    dma_channel_config cfg = dma_channel_get_default_config(dma_mem_channel);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, false); // Read from a static address (fill_value)
    channel_config_set_write_increment(&cfg, true);  // Increment destination
    
    dma_channel_set_config(dma_mem_channel, &cfg, false);
}

void dma_mem_fill32(uint32_t *dest, uint32_t value, uint32_t count) {
    // Ensure previous transfer is done
    dma_mem_wait();
    
    fill_value = value;
    dma_channel_set_read_addr(dma_mem_channel, &fill_value, false);
    dma_channel_set_write_addr(dma_mem_channel, dest, false);
    dma_channel_set_trans_count(dma_mem_channel, count, true);
}

bool dma_mem_is_busy(void) {
    if (dma_mem_channel == -1) return false;
    return dma_channel_is_busy(dma_mem_channel);
}

void dma_mem_wait(void) {
    if (dma_mem_channel == -1) return;
    dma_channel_wait_for_finish_blocking(dma_mem_channel);
}
