#include "system_config.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"

static uint32_t actual_spi_hz = 0;

void system_init(const system_config_t *config) {
    // Set voltage for stability at higher clocks
    if (config->cpu_mhz >= 200) {
        vreg_set_voltage(VREG_VOLTAGE_1_20);
    }
    if (config->cpu_mhz >= 250) {
        vreg_set_voltage(VREG_VOLTAGE_1_25);
    }
    if (config->cpu_mhz >= 300) {
        vreg_set_voltage(VREG_VOLTAGE_1_30);
    }
    sleep_ms(10);
    
    // Set CPU clock
    set_sys_clock_khz(config->cpu_mhz * 1000, true);
    
    // Configure peripheral clock to match system clock
    uint32_t freq = clock_get_hz(clk_sys);
    clock_configure(clk_peri, 0, 
                   CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, 
                   freq, freq);
}

uint32_t system_get_cpu_hz(void) {
    return clock_get_hz(clk_sys);
}

uint32_t system_get_spi_hz(void) {
    return actual_spi_hz;
}

void system_set_actual_spi_hz(uint32_t hz) {
    actual_spi_hz = hz;
}
