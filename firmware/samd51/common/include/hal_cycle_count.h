#ifndef HAL_CYCLE_COUNT_H
#define HAL_CYCLE_COUNT_H

#include "sam.h"
#include <stdint.h>

/**
 * DWT cycle counter access for Cortex-M3/M4/M7.
 *
 * The DWT CYCCNT register counts CPU clock cycles as a free-running
 * 32-bit counter. At 48 MHz it wraps every ~89 seconds; at 120 MHz
 * every ~35 seconds. Useful for sub-microsecond profiling and precise
 * short delays.
 *
 * Call hal_cycle_count_init() once at startup to enable the counter.
 */

/** Enable the DWT cycle counter. Must be called before reading CYCCNT. */
static inline void hal_cycle_count_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  /* enable trace */
    DWT->CYCCNT = 0U;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;             /* start counting */
}

/** Read the current DWT cycle count. */
static inline uint32_t hal_cycle_count_get(void)
{
    return DWT->CYCCNT;
}

/**
 * Compute elapsed cycles between two readings.
 * Handles single wrap-around correctly (uint32_t subtraction).
 */
static inline uint32_t hal_cycle_count_elapsed(uint32_t start, uint32_t end)
{
    return end - start;
}

#endif /* HAL_CYCLE_COUNT_H */
