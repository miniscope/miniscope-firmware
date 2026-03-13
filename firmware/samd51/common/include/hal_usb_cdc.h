#ifndef HAL_USB_CDC_H
#define HAL_USB_CDC_H

/**
 * USB CDC (Communications Device Class) HAL.
 *
 * Provides a simple serial-over-USB interface using TinyUSB.
 * When FW_ENABLE_USB is not defined, all functions compile to
 * zero-cost inline no-ops.
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef FW_ENABLE_USB

/** Initialize USB clocks, pins, and TinyUSB stack. Call once from board_init(). */
void hal_usb_cdc_init(void);

/** Process TinyUSB device events. Call from the main loop. */
void hal_usb_cdc_task(void);

/** Return true if a CDC host is connected. */
bool hal_usb_cdc_connected(void);

/** Write a null-terminated string to the CDC port and flush. */
void hal_usb_cdc_puts(const char *str);

/** Write a buffer to the CDC port and flush. */
void hal_usb_cdc_write(const void *data, uint32_t len);

#else /* !FW_ENABLE_USB */

static inline void hal_usb_cdc_init(void) {}
static inline void hal_usb_cdc_task(void) {}
static inline bool hal_usb_cdc_connected(void) { return false; }
static inline void hal_usb_cdc_puts(const char *str) { (void)str; }
static inline void hal_usb_cdc_write(const void *data, uint32_t len) { (void)data; (void)len; }

#endif /* FW_ENABLE_USB */
#endif /* HAL_USB_CDC_H */
