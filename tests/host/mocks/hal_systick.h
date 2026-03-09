#ifndef HAL_SYSTICK_H
#define HAL_SYSTICK_H

/**
 * Mock SysTick HAL for host-based testing.
 *
 * hal_delay_ms() is a no-op counter so tests can verify it was called
 * without actually blocking.
 */

#include <stdint.h>

extern uint32_t mock_delay_ms_total;

static inline void hal_systick_init(uint32_t cpu_hz)
{
    (void)cpu_hz;
}

static inline uint32_t hal_systick_get_ticks(void)
{
    return 0;
}

static inline void hal_delay_ms(uint32_t ms)
{
    mock_delay_ms_total += ms;
}

#endif /* HAL_SYSTICK_H */
