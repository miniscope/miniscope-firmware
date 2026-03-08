#ifndef HAL_DELAY_H
#define HAL_DELAY_H

#include <stdint.h>

/**
 * Approximate busy-wait delay for SAMD51.
 *
 * Calibrated for the default 48 MHz DFLL clock. This is a rough delay
 * suitable for blinking LEDs and non-critical timing. For precision timing,
 * use a hardware timer (TC/TCC peripheral).
 */

#define HAL_DELAY_LOOPS_PER_MS 6000u

static inline void delay_ms_approx(uint32_t ms)
{
    for (uint32_t i = 0; i < ms * HAL_DELAY_LOOPS_PER_MS; i++) {
        __asm__ volatile("nop");
    }
}

#endif /* HAL_DELAY_H */
