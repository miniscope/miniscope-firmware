#ifdef FW_ENABLE_USB

#include "tusb.h"
#include <stdint.h>

/* --------------------------------------------------------------------------
 * Device Descriptor
 * -------------------------------------------------------------------------- */
static const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0xCafe,
    .idProduct          = 0x4001,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1,
};

const uint8_t *tud_descriptor_device_cb(void)
{
    return (const uint8_t *)&desc_device;
}

/* --------------------------------------------------------------------------
 * Configuration Descriptor
 * -------------------------------------------------------------------------- */
enum {
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)
#define EPNUM_CDC_NOTIF   0x81
#define EPNUM_CDC_OUT     0x02
#define EPNUM_CDC_IN      0x82

static const uint8_t desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          0x00, 100),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8,
                       EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
};

const uint8_t *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_configuration;
}

/* --------------------------------------------------------------------------
 * String Descriptors
 * -------------------------------------------------------------------------- */

/* Language ID */
static const uint16_t string_desc_lang[] = { 0x0304, 0x0409 };

/* Convert ASCII string to UTF-16LE descriptor in a static buffer */
static uint16_t _desc_buf[32 + 1];

static const uint16_t *ascii_to_utf16(const char *str)
{
    uint8_t len = 0;
    while (str[len] && len < 31) {
        _desc_buf[1 + len] = str[len];
        len++;
    }
    _desc_buf[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * len + 2));
    return _desc_buf;
}

/* SAMD51 128-bit die serial number — 4 words at known NVM addresses */
static const uint32_t *const serial_word_addr[4] = {
    (const uint32_t *)0x008061FCUL,
    (const uint32_t *)0x00806010UL,
    (const uint32_t *)0x00806014UL,
    (const uint32_t *)0x00806018UL,
};

static const char hex_chars[] = "0123456789ABCDEF";

/**
 * Read the MCU's unique 128-bit serial and format as 32-char hex string.
 */
static const uint16_t *get_serial_string(void)
{
    uint8_t pos = 0;
    for (int w = 0; w < 4; w++) {
        uint32_t val = *serial_word_addr[w];
        for (int b = 28; b >= 0; b -= 4) {
            if (pos >= 31) break;
            _desc_buf[1 + pos] = hex_chars[(val >> b) & 0x0F];
            pos++;
        }
    }
    _desc_buf[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * pos + 2));
    return _desc_buf;
}

const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;

    switch (index) {
    case 0:
        return string_desc_lang;
    case 1:
        return ascii_to_utf16("Miniscope");
    case 2:
        return ascii_to_utf16("Miniscope CDC");
    case 3:
        return get_serial_string();
    case 4:
        return ascii_to_utf16("Miniscope CDC Port");
    default:
        return NULL;
    }
}

#endif /* FW_ENABLE_USB */
