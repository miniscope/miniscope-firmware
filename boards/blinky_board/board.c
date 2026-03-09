#include "board.h"
#include "blinky_config.h"
#include "hal_gpio.h"
#include "hal_clock.h"

void board_init(void)
{
    /* Initialize the clock tree first — must happen before any peripheral
     * or timer init that depends on knowing the CPU frequency. */
    hal_clock_init_preset(HAL_CLOCK_PRESET);

    /* Configure LED pin as output */
    gpio_set_output(BLINKY_LED_PORT, BLINKY_LED_PIN);
    gpio_clear(BLINKY_LED_PORT, BLINKY_LED_PIN);
}
