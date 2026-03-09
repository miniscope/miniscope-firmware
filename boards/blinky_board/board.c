#include "board.h"
#include "fw_config.h"
#include "blinky_config.h"
#include "hal_gpio.h"
#include "hal_clock.h"
#include "hal_systick.h"
#include "hal_cycle_count.h"

void board_init(void)
{
    /* Initialize the clock tree first — must happen before any peripheral
     * or timer init that depends on knowing the CPU frequency. */
    hal_clock_init_preset(HAL_CLOCK_PRESET);

    /* Initialize system timing */
    hal_systick_init(F_CPU);
    hal_cycle_count_init();

    /* Configure LED pin as output */
    gpio_set_output(BLINKY_LED_PORT, BLINKY_LED_PIN);
    gpio_clear(BLINKY_LED_PORT, BLINKY_LED_PIN);
}
