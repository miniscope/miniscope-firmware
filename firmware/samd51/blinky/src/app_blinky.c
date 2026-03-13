#include "app.h"
#include "blinky_config.h"
#include "hal_systick.h"
#include "hal_gpio.h"
#include "hal_usb_cdc.h"

#ifdef FW_ENABLE_USB
#include "fw_config.h"
#include "fw_version.h"
#include <string.h>
#endif

static uint32_t last_blink_tick;
static uint32_t blink_count;

#ifdef FW_ENABLE_USB
static bool boot_msg_sent;
static uint32_t last_heartbeat_tick;

/**
 * Append decimal representation of val to dst, return pointer past last char.
 */
static char *append_u32(char *dst, uint32_t val)
{
    char tmp[10]; /* max 10 digits for uint32 */
    int i = 0;
    if (val == 0) {
        *dst++ = '0';
        return dst;
    }
    while (val > 0) {
        tmp[i++] = '0' + (char)(val % 10);
        val /= 10;
    }
    while (i > 0) {
        *dst++ = tmp[--i];
    }
    return dst;
}

static char *append_str(char *dst, const char *src)
{
    while (*src) {
        *dst++ = *src++;
    }
    return dst;
}
#endif

void app_init(void)
{
    last_blink_tick = hal_systick_get_ticks();
    blink_count = 0;
#ifdef FW_ENABLE_USB
    boot_msg_sent = false;
    last_heartbeat_tick = hal_systick_get_ticks();
#endif
}

void app_run(void)
{
    hal_usb_cdc_task();

    uint32_t now = hal_systick_get_ticks();

    /* Non-blocking LED toggle */
    if ((now - last_blink_tick) >= BLINKY_DELAY_MS) {
        gpio_toggle(BLINKY_LED_PORT, BLINKY_LED_PIN);
        last_blink_tick += BLINKY_DELAY_MS;
        blink_count++;
    }

#ifdef FW_ENABLE_USB
    if (hal_usb_cdc_connected()) {
        if (!boot_msg_sent) {
            char buf[128];
            char *p = buf;
            p = append_str(p, "[BOOT] miniscope-fw ");
            p = append_str(p, FW_VERSION_STRING);
            p = append_str(p, " board=");
            p = append_str(p, FW_BOARD_NAME);
            p = append_str(p, " mcu=");
            p = append_str(p, FW_BOARD_MCU);
            p = append_str(p, "\r\n");
            *p = '\0';
            hal_usb_cdc_puts(buf);
            boot_msg_sent = true;
        }

        if ((now - last_heartbeat_tick) >= 1000) {
            char buf[64];
            char *p = buf;
            p = append_str(p, "[BLINK] count=");
            p = append_u32(p, blink_count);
            p = append_str(p, " uptime=");
            p = append_u32(p, now);
            p = append_str(p, "\r\n");
            *p = '\0';
            hal_usb_cdc_puts(buf);
            last_heartbeat_tick += 1000;
        }
    }
#endif
}
