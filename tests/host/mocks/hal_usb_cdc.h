#ifndef HAL_USB_CDC_H
#define HAL_USB_CDC_H

/**
 * Mock USB CDC HAL for host-based testing.
 *
 * Provides observable state so tests can verify USB CDC operations.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define MOCK_USB_CDC_BUF_SIZE 1024

extern bool mock_usb_cdc_connected;
extern char mock_usb_cdc_output[MOCK_USB_CDC_BUF_SIZE];
extern int  mock_usb_cdc_output_pos;
extern int  mock_usb_cdc_task_count;

static inline void hal_usb_cdc_init(void) {}

static inline void hal_usb_cdc_task(void)
{
    mock_usb_cdc_task_count++;
}

static inline bool hal_usb_cdc_connected(void)
{
    return mock_usb_cdc_connected;
}

static inline void hal_usb_cdc_puts(const char *str)
{
    int len = (int)strlen(str);
    if (mock_usb_cdc_output_pos + len < MOCK_USB_CDC_BUF_SIZE) {
        memcpy(&mock_usb_cdc_output[mock_usb_cdc_output_pos], str, (size_t)len);
        mock_usb_cdc_output_pos += len;
        mock_usb_cdc_output[mock_usb_cdc_output_pos] = '\0';
    }
}

static inline void hal_usb_cdc_write(const void *data, uint32_t len)
{
    (void)data;
    (void)len;
}

#endif /* HAL_USB_CDC_H */
