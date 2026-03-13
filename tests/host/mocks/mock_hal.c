#include "hal_gpio.h"
#include "hal_systick.h"
#include "hal_usb_cdc.h"

/* Mock GPIO state */
uint32_t mock_gpio_dir[MOCK_GPIO_MAX_PORTS];
uint32_t mock_gpio_out[MOCK_GPIO_MAX_PORTS];

/* Mock SysTick state */
uint32_t mock_delay_ms_total;
uint32_t mock_tick_value;

/* Mock USB CDC state */
bool mock_usb_cdc_connected;
char mock_usb_cdc_output[MOCK_USB_CDC_BUF_SIZE];
int  mock_usb_cdc_output_pos;
int  mock_usb_cdc_task_count;
