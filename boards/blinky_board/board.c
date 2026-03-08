#include "board.h"
#include "blinky_config.h"
#include "hal_gpio.h"

void board_init(void)
{
    /* Configure LED pin as output */
    gpio_set_output(BLINKY_LED_PORT, BLINKY_LED_PIN);
    gpio_clear(BLINKY_LED_PORT, BLINKY_LED_PIN);
}
