#include "board.h"
#include "fw_config.h"
#include "hal_gpio.h"
#include "hal_clock.h"
#include "hal_systick.h"
#include "hal_cycle_count.h"

void board_init(void)
{
    /* Initialize the clock tree — 60 MHz CPU via DPLL0, XOSC32K reference */
    hal_clock_init_preset(HAL_CLOCK_PRESET);

    /* Initialize system timing */
    hal_systick_init(F_CPU);
    hal_cycle_count_init();

    /* Enable 3.3V power rail */
    gpio_set_output(BOARD_EN_3V3_PORT, BOARD_EN_3V3_PIN);
    gpio_set(BOARD_EN_3V3_PORT, BOARD_EN_3V3_PIN);

    /* Status LED */
    gpio_set_output(BOARD_LED_PORT, BOARD_LED_PIN);
    gpio_clear(BOARD_LED_PORT, BOARD_LED_PIN);

    /*
     * Sensor clock: GCLK1 output on PB15.
     * DPLL0 = 120 MHz, GCLK1 div=6 → 20 MHz sensor clock.
     */
    hal_gclk_gen_config_t gclk1_cfg = {
        .gen = 1,
        .src = HAL_CLK_SRC_DPLL0,
        .div = 6,
        .idc = true,
        .oe  = true,
    };
    hal_clock_configure_gclk_gen(&gclk1_cfg);

    /* Route GCLK1 to PB15 (PMUX M = peripheral function M) */
    gpio_set_pmux(BOARD_SCLK_PORT, BOARD_SCLK_PIN, PORT_PMUX_PMUXO_M_Val);
}
