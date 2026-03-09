#include "app.h"
#include "blinky_config.h"
#include "hal_systick.h"
#include "hal_gpio.h"

void app_init(void)
{
    /* LED pin direction is configured by board_init() */
}

void app_run(void)
{
    gpio_toggle(BLINKY_LED_PORT, BLINKY_LED_PIN);
    hal_delay_ms(BLINKY_DELAY_MS);
}
