#include "hal_usb_cdc.h"

#ifdef FW_ENABLE_USB

#include "sam.h"
#include "tusb.h"
#include "hal_clock.h"
#include "hal_gpio.h"
#include <string.h>

void hal_usb_cdc_init(void)
{
    /* Enable USB bus clocks */
    MCLK_REGS->MCLK_AHBMASK |= MCLK_AHBMASK_USB_Msk;
    hal_clock_enable_apb(&MCLK_REGS->MCLK_APBBMASK, MCLK_APBBMASK_USB_Msk);

    /* Route 48 MHz GCLK0 to USB peripheral channel (channel 10) */
    hal_clock_enable_gclk_channel(10, 0);

    /* Configure PA24 (D-) and PA25 (D+) to USB peripheral function H (mux 7) */
    gpio_set_pmux(0, 24, 7);
    gpio_set_pmux(0, 25, 7);

    /* Initialize TinyUSB */
    tusb_init();
}

void hal_usb_cdc_task(void)
{
    tud_task();
}

bool hal_usb_cdc_connected(void)
{
    return tud_cdc_connected();
}

void hal_usb_cdc_puts(const char *str)
{
    uint32_t len = (uint32_t)strlen(str);
    if (len > 0) {
        tud_cdc_write(str, len);
        tud_cdc_write_flush();
    }
}

void hal_usb_cdc_write(const void *data, uint32_t len)
{
    if (len > 0) {
        tud_cdc_write(data, len);
        tud_cdc_write_flush();
    }
}

#endif /* FW_ENABLE_USB */
