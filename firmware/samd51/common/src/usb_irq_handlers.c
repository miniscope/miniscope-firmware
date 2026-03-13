#ifdef FW_ENABLE_USB

#include "tusb.h"

/**
 * USB interrupt handlers for SAMD51 (DFP v3 naming).
 *
 * The SAMD51 has four USB interrupt vectors. All forward to TinyUSB's
 * dcd_int_handler(). These override the weak Dummy_Handler aliases
 * defined by the DFP startup file.
 */

void USB_OTHER_Handler(void)    { tud_int_handler(0); }
void USB_SOF_HSOF_Handler(void) { tud_int_handler(0); }
void USB_TRCPT0_Handler(void)   { tud_int_handler(0); }
void USB_TRCPT1_Handler(void)   { tud_int_handler(0); }

#endif /* FW_ENABLE_USB */
