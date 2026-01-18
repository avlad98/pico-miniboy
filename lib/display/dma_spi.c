#include "dma_spi.h"
#include "hardware/dma.h"
#include "hardware/pio.h"

static int dma_channel = -1;
static spi_inst_t *dma_spi_instance = NULL;

void dma_spi_init(spi_inst_t *spi) {
    // Note: spi argument is nowignored as we use PIO0 / SM 0
    
    // Claim DMA channel
    dma_channel = dma_claim_unused_channel(true);
    
    // Configure DMA
    dma_channel_config cfg = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_dreq(&cfg, pio_get_dreq(pio0, 0, true));
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    
    dma_channel_set_config(dma_channel, &cfg, false);
}

void dma_spi_transfer(const uint8_t *data, uint32_t len) {
    dma_channel_set_read_addr(dma_channel, data, false);
    // Write to the TOP byte of the FIFO register for correct PIO bit alignment
    dma_channel_set_write_addr(dma_channel, (uint8_t*)&pio0->txf[0] + 3, false);
    dma_channel_set_trans_count(dma_channel, len, true);  // true = start immediately
}

void dma_spi_wait(void) {
    dma_channel_wait_for_finish_blocking(dma_channel);
    
    // Wait for PIO TX FIFO to drain
    while (!pio_sm_is_tx_fifo_empty(pio0, 0)) {
        tight_loop_contents();
    }
    // Final wait for the internal OSR (Output Shift Register) to finish.
    // At 100MHz, this is 16 cycles at 200MHz. 100 NOPs is plenty.
    for (int i = 0; i < 100; i++) __asm volatile ("nop");
}

bool dma_spi_is_busy(void) {
    return dma_channel_is_busy(dma_channel) || !pio_sm_is_tx_fifo_empty(pio0, 0);
}
