#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

/**
 * TinyUSB configuration for SAMD51 boards.
 *
 * Entire config is gated on FW_ENABLE_USB. When disabled, CFG_TUD_ENABLED=0
 * causes TinyUSB to compile to nothing.
 */

#ifdef FW_ENABLE_USB

#define CFG_TUSB_MCU              OPT_MCU_SAMD51
#define CFG_TUSB_OS               OPT_OS_NONE
#define CFG_TUSB_RHPORT0_MODE     (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

#define CFG_TUD_ENABLED           1

/* Class enables — CDC only */
#define CFG_TUD_CDC               1
#define CFG_TUD_MSC               0
#define CFG_TUD_HID               0
#define CFG_TUD_MIDI              0
#define CFG_TUD_VENDOR            0

/* CDC FIFO sizes */
#define CFG_TUD_CDC_RX_BUFSIZE    256
#define CFG_TUD_CDC_TX_BUFSIZE    256

#else /* !FW_ENABLE_USB */

#define CFG_TUD_ENABLED           0

#endif /* FW_ENABLE_USB */
#endif /* TUSB_CONFIG_H */
