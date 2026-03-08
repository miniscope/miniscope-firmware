#include "app.h"
#include "blinky_config.h"
#include "hal_gpio.h"

/* Approximate loop iterations per millisecond at 48 MHz DFLL default clock */
#define DELAY_LOOPS_PER_MS 6000u

static void delay_ms_approx(uint32_t ms)
{
    for (uint32_t i = 0; i < ms * DELAY_LOOPS_PER_MS; i++) {
        __asm__ volatile("nop");
    }
}

void app_init(void)
{
    /* LED pin direction is configured by board_init() */
}

void app_run(void)
{
    gpio_toggle(BLINKY_LED_PORT, BLINKY_LED_PIN);
    delay_ms_approx(BLINKY_DELAY_MS);
}
