#include "dma_spi.h"
#include "hardware/dma.h"

static int dma_channel = -1;
static spi_inst_t *dma_spi_instance = NULL;

void dma_spi_init(spi_inst_t *spi) {
    dma_spi_instance = spi;
    
    // Claim DMA channel
    dma_channel = dma_claim_unused_channel(true);
    
    // Configure DMA
    dma_channel_config cfg = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_dreq(&cfg, spi_get_dreq(spi, true));
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    
    dma_channel_set_config(dma_channel, &cfg, false);
}

void dma_spi_transfer(const uint8_t *data, uint32_t len) {
    dma_channel_set_read_addr(dma_channel, data, false);
    dma_channel_set_write_addr(dma_channel, &spi_get_hw(dma_spi_instance)->dr, false);
    dma_channel_set_trans_count(dma_channel, len, true);  // true = start immediately
}

void dma_spi_wait(void) {
    dma_channel_wait_for_finish_blocking(dma_channel);
    
    // Wait for SPI FIFO to drain
    while (spi_is_busy(dma_spi_instance)) {
        tight_loop_contents();
    }
}

bool dma_spi_is_busy(void) {
    return dma_channel_is_busy(dma_channel) || spi_is_busy(dma_spi_instance);
}
