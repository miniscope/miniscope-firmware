#ifdef FW_ENABLE_USB

#include "tusb.h"

/**
 * USB interrupt handlers for SAMD51.
 *
 * The SAMD51 has four USB interrupt vectors. All forward to TinyUSB's
 * dcd_int_handler(). These override the weak Dummy_Handler aliases
 * defined by the DFP startup file.
 */

void USB_0_Handler(void) { tud_int_handler(0); }
void USB_1_Handler(void) { tud_int_handler(0); }
void USB_2_Handler(void) { tud_int_handler(0); }
void USB_3_Handler(void) { tud_int_handler(0); }

#endif /* FW_ENABLE_USB */
