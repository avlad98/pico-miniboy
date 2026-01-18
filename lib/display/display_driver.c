#include "display_driver.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "spi.pio.h"

static PIO pio_instance = pio0;
static uint sm_instance = 0;
static uint pio_offset = 0;

static display_config_t current_display;

void display_init(const display_config_t *config, const system_config_t *sys_config) {
    current_display = *config;
    
    // GPIO setup
    gpio_init(config->pin_cs);
    gpio_set_dir(config->pin_cs, GPIO_OUT);
    gpio_put(config->pin_cs, 1);
    
    gpio_init(config->pin_rst);
    gpio_set_dir(config->pin_rst, GPIO_OUT);
    
    gpio_init(config->pin_dc);
    gpio_set_dir(config->pin_dc, GPIO_OUT);
    
    gpio_init(config->pin_bl);
    gpio_set_dir(config->pin_bl, GPIO_OUT);
    
    // Initialize PIO for SPI
    pio_offset = pio_add_program(pio_instance, &spi_tx_program);
    
    // SPI clock in PIO 2-cycle loop is SysClk / (div * 2)
    float div_init = (float)system_get_cpu_hz() / (sys_config->spi_hz_init * 2);
    spi_tx_init(pio_instance, sm_instance, pio_offset, config->pin_sck, config->pin_mosi, div_init);
    
    // Improve signal integrity for high-speed SPI
    gpio_set_drive_strength(config->pin_sck, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_slew_rate(config->pin_sck, GPIO_SLEW_RATE_FAST);
    gpio_set_drive_strength(config->pin_mosi, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_slew_rate(config->pin_mosi, GPIO_SLEW_RATE_FAST);
    gpio_set_drive_strength(config->pin_cs, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_slew_rate(config->pin_cs, GPIO_SLEW_RATE_FAST);
    
    // Hardware reset
    gpio_put(config->pin_rst, 0);
    sleep_ms(10);
    gpio_put(config->pin_rst, 1);
    sleep_ms(120);
    
    // ILI9341 init sequence
    display_cmd(0x01); // Software reset
    sleep_ms(150);
    
    display_cmd(0x11); // Sleep out
    sleep_ms(10);
    
    display_cmd(0x36); // Memory Access Control
    display_data(0x68);
    
    display_cmd(0x3A); // Pixel Format
    display_data(config->format);
    
    display_cmd(0x29); // Display ON
    
    // Speed up PIO for framebuffer transfers
    float div_fast = (float)system_get_cpu_hz() / (sys_config->spi_hz_fast * 2);
    pio_sm_set_clkdiv(pio_instance, sm_instance, div_fast);
    system_set_actual_spi_hz(sys_config->spi_hz_fast);
    
    // Backlight on
    gpio_put(config->pin_bl, 1);
}

void display_cmd(uint8_t cmd) {
    gpio_put(current_display.pin_dc, 0);
    gpio_put(current_display.pin_cs, 0);
    
    // Write 8 bits to the MSB of the 32-bit FIFO for correct alignment
    *((io_rw_8 *)&pio_instance->txf[sm_instance] + 3) = cmd;
    
    while (!pio_sm_is_tx_fifo_empty(pio_instance, sm_instance));
    // 2-cycle loop at 10Mhz needs ~200 NOPs for safety
    for (int i = 0; i < 200; i++) __asm volatile ("nop");
    
    gpio_put(current_display.pin_cs, 1);
}

void display_data(uint8_t data) {
    gpio_put(current_display.pin_dc, 1);
    gpio_put(current_display.pin_cs, 0);
    
    *((io_rw_8 *)&pio_instance->txf[sm_instance] + 3) = data;
    
    while (!pio_sm_is_tx_fifo_empty(pio_instance, sm_instance));
    for (int i = 0; i < 200; i++) __asm volatile ("nop");
    
    gpio_put(current_display.pin_cs, 1);
}

void display_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Column address
    display_cmd(0x2A);
    display_data(x0 >> 8);
    display_data(x0 & 0xFF);
    display_data(x1 >> 8);
    display_data(x1 & 0xFF);
    
    // Row address
    display_cmd(0x2B);
    display_data(y0 >> 8);
    display_data(y0 & 0xFF);
    display_data(y1 >> 8);
    display_data(y1 & 0xFF);
    
    // Write to RAM
    display_cmd(0x2C);
}

void display_start_write(void) {
    gpio_put(current_display.pin_dc, 1);
    gpio_put(current_display.pin_cs, 0);
}

void display_end_write(void) {
    gpio_put(current_display.pin_cs, 1);
}

uint16_t display_get_width(void) {
    return current_display.width;
}

uint16_t display_get_height(void) {
    return current_display.height;
}

spi_inst_t* display_get_spi(void) {
    return current_display.spi;
}
