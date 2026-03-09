#include "hal_gpio.h"
#include "hal_systick.h"

/* Mock GPIO state */
uint32_t mock_gpio_dir[MOCK_GPIO_MAX_PORTS];
uint32_t mock_gpio_out[MOCK_GPIO_MAX_PORTS];

/* Mock SysTick state */
uint32_t mock_delay_ms_total;
