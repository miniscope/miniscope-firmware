#ifndef HAL_SYSTICK_H
#define HAL_SYSTICK_H

#include <stdint.h>

/**
 * SysTick millisecond tick and blocking delay for Cortex-M.
 *
 * Call hal_systick_init() once at startup before using any other function
 * in this header. The SysTick interrupt fires every 1 ms and increments
 * an internal counter used by hal_systick_get_ticks() and hal_delay_ms().
 */

/**
 * Configure SysTick for a 1 ms tick interval.
 *
 * @param cpu_hz  CPU / SysTick clock frequency in Hz.
 *                For default SAMD51 boot this is 48000000.
 */
void hal_systick_init(uint32_t cpu_hz);

/** Return milliseconds elapsed since hal_systick_init(). */
uint32_t hal_systick_get_ticks(void);

/**
 * Blocking delay using the SysTick tick counter.
 *
 * Accuracy is +0/-1 ms (depending on where in the current tick the call
 * is made). Safe to call from thread context; do not call from ISRs.
 *
 * @param ms  Delay in milliseconds.
 */
void hal_delay_ms(uint32_t ms);

#endif /* HAL_SYSTICK_H */
