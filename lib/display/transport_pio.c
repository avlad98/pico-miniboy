#include "transport_pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "spi.pio.h"
#include <stdlib.h>

typedef struct {
  transport_pio_config_t cfg;
  bool is_fast;
} transport_pio_priv_t;

static void pio_wait_idle(PIO pio, uint sm) {
  while (!pio_sm_is_tx_fifo_empty(pio, sm))
    ;
  uint32_t stall_mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + sm);
  pio->fdebug = stall_mask;
  while (!(pio->fdebug & stall_mask))
    ;
}

static void transport_pio_init(display_transport_t *self,
                               uint32_t speed_init_hz, uint32_t speed_fast_hz) {
  transport_pio_priv_t *priv = (transport_pio_priv_t *)self->priv;

  gpio_init(priv->cfg.pin_cs);
  gpio_set_dir(priv->cfg.pin_cs, GPIO_OUT);
  gpio_put(priv->cfg.pin_cs, 1);

  gpio_init(priv->cfg.pin_dc);
  gpio_set_dir(priv->cfg.pin_dc, GPIO_OUT);

  priv->cfg.pio_offset = pio_add_program(priv->cfg.pio, &spi_tx_program);

  uint32_t sys_hz = clock_get_hz(clk_sys);

  priv->cfg.div_init = (float)sys_hz / (speed_init_hz * 2);
  priv->cfg.div_fast = (float)sys_hz / (speed_fast_hz * 2);

  spi_tx_init(priv->cfg.pio, priv->cfg.sm, priv->cfg.pio_offset,
              priv->cfg.pin_sck, priv->cfg.pin_mosi, priv->cfg.div_init);

  gpio_set_drive_strength(priv->cfg.pin_sck, GPIO_DRIVE_STRENGTH_12MA);
  gpio_set_slew_rate(priv->cfg.pin_sck, GPIO_SLEW_RATE_FAST);
  gpio_set_drive_strength(priv->cfg.pin_mosi, GPIO_DRIVE_STRENGTH_12MA);
  gpio_set_slew_rate(priv->cfg.pin_mosi, GPIO_SLEW_RATE_FAST);
  gpio_set_drive_strength(priv->cfg.pin_cs, GPIO_DRIVE_STRENGTH_12MA);
  gpio_set_slew_rate(priv->cfg.pin_cs, GPIO_SLEW_RATE_FAST);

  // DMA Setup
  priv->cfg.dma_chan = dma_claim_unused_channel(true);
  dma_channel_config c = dma_channel_get_default_config(priv->cfg.dma_chan);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
  channel_config_set_dreq(&c, pio_get_dreq(priv->cfg.pio, priv->cfg.sm, true));
  channel_config_set_read_increment(&c, true);
  channel_config_set_write_increment(&c, false);
  channel_config_set_high_priority(&c, true);
  dma_channel_set_config(priv->cfg.dma_chan, &c, false);
}

static void transport_pio_set_speed(display_transport_t *self, bool fast) {
  transport_pio_priv_t *priv = (transport_pio_priv_t *)self->priv;
  priv->is_fast = fast;
  pio_sm_set_clkdiv(priv->cfg.pio, priv->cfg.sm,
                    fast ? priv->cfg.div_fast : priv->cfg.div_init);
}

static void transport_pio_send_cmd(display_transport_t *self, uint8_t cmd) {
  transport_pio_priv_t *priv = (transport_pio_priv_t *)self->priv;
  gpio_put(priv->cfg.pin_dc, 0);
  gpio_put(priv->cfg.pin_cs, 0);
  *((io_rw_8 *)&priv->cfg.pio->txf[priv->cfg.sm] + 3) = cmd;
  pio_wait_idle(priv->cfg.pio, priv->cfg.sm);
  gpio_put(priv->cfg.pin_cs, 1);
}

static void transport_pio_send_data8(display_transport_t *self, uint8_t data) {
  transport_pio_priv_t *priv = (transport_pio_priv_t *)self->priv;
  gpio_put(priv->cfg.pin_dc, 1);
  gpio_put(priv->cfg.pin_cs, 0);
  *((io_rw_8 *)&priv->cfg.pio->txf[priv->cfg.sm] + 3) = data;
  pio_wait_idle(priv->cfg.pio, priv->cfg.sm);
  gpio_put(priv->cfg.pin_cs, 1);
}

static void transport_pio_send_buffer(display_transport_t *self,
                                      const uint8_t *data, uint32_t len) {
  transport_pio_priv_t *priv = (transport_pio_priv_t *)self->priv;
  gpio_put(priv->cfg.pin_dc, 1);
  gpio_put(priv->cfg.pin_cs, 0);

  dma_channel_set_read_addr(priv->cfg.dma_chan, data, false);
  dma_channel_set_write_addr(priv->cfg.dma_chan,
                             (uint8_t *)&priv->cfg.pio->txf[priv->cfg.sm] + 3,
                             false);
  dma_channel_set_trans_count(priv->cfg.dma_chan, len, true);
}

static void transport_pio_wait(display_transport_t *self) {
  transport_pio_priv_t *priv = (transport_pio_priv_t *)self->priv;
  dma_channel_wait_for_finish_blocking(priv->cfg.dma_chan);
  pio_wait_idle(priv->cfg.pio, priv->cfg.sm);
  gpio_put(priv->cfg.pin_cs, 1);
}

static bool transport_pio_is_busy(display_transport_t *self) {
  transport_pio_priv_t *priv = (transport_pio_priv_t *)self->priv;
  return dma_channel_is_busy(priv->cfg.dma_chan) ||
         !pio_sm_is_tx_fifo_empty(priv->cfg.pio, priv->cfg.sm);
}

display_transport_t *
transport_pio_create(const transport_pio_config_t *config) {
  display_transport_t *t = malloc(sizeof(display_transport_t));
  transport_pio_priv_t *priv = malloc(sizeof(transport_pio_priv_t));

  priv->cfg = *config;
  priv->is_fast = false;

  t->init = transport_pio_init;
  t->set_speed = transport_pio_set_speed;
  t->send_cmd = transport_pio_send_cmd;
  t->send_data8 = transport_pio_send_data8;
  t->send_buffer = transport_pio_send_buffer;
  t->wait = transport_pio_wait;
  t->is_busy = transport_pio_is_busy;
  t->priv = priv;

  return t;
}
