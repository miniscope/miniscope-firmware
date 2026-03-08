#include "app.h"
#include "blinky_config.h"
#include "hal_gpio.h"

static void delay_ms_approx(uint32_t ms)
{
    /* Rough busy-wait delay assuming ~48 MHz default clock.
     * Not accurate - just enough for a visible blink. */
    for (volatile uint32_t i = 0; i < ms * 6000u; i++) {
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
