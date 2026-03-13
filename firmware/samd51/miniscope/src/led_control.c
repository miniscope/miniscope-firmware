#include "fw_config.h"

#ifdef FW_ENABLE_LED_PWM

#include "led_control.h"
#include "board.h"
#include "hal_gpio.h"
#include "hal_tc.h"
#include "hal_clock.h"

/*
 * TC0 PWM for excitation LED on PB30 (TC0/WO[0]).
 *
 * TC0 is on APB A bus. GCLK channel: TC0_GCLK_ID (9).
 * Using GCLK0 (120 MHz) with TC prescaler DIV64:
 *   TC clock = 120 MHz / 64 = 1.875 MHz
 *   Period = 1875 → ~1 kHz PWM frequency
 */

#define LED_PWM_PERIOD 1875

void led_control_init(void)
{
    /* Enable line as output, initially off */
    gpio_set_output(BOARD_EX_LED_EN_PORT, BOARD_EX_LED_EN_PIN);
    gpio_clear(BOARD_EX_LED_EN_PORT, BOARD_EX_LED_EN_PIN);

    /* Route PB30 to TC0/WO[0] — PMUX E */
    gpio_set_pmux(BOARD_EX_LED_PORT, BOARD_EX_LED_PIN, PORT_PMUX_PMUXE_E_Val);

    /* Enable MCLK for TC0 (APB A bus) */
    hal_clock_enable_apb(&MCLK_REGS->MCLK_APBAMASK, MCLK_APBAMASK_TC0_Msk);

    /* Route GCLK0 to TC0 */
    hal_clock_enable_gclk_channel(TC0_GCLK_ID, 0);

    /* Initialize TC0 in NPWM mode, prescaler DIV64 */
    hal_tc_init(TC0_REGS, HAL_TC_MODE_NPWM, 5); /* 5 = DIV64 */
    hal_tc_set_period(TC0_REGS, LED_PWM_PERIOD);
    hal_tc_set_compare(TC0_REGS, 0); /* Start with LED off */
}

void led_control_set_duty(uint8_t duty)
{
    hal_tc_set_duty_percent(TC0_REGS, duty);
}

void led_control_enable(void)
{
    gpio_set(BOARD_EX_LED_EN_PORT, BOARD_EX_LED_EN_PIN);
    hal_tc_start(TC0_REGS);
}

void led_control_disable(void)
{
    hal_tc_stop(TC0_REGS);
    gpio_clear(BOARD_EX_LED_EN_PORT, BOARD_EX_LED_EN_PIN);
}

#endif /* FW_ENABLE_LED_PWM */
