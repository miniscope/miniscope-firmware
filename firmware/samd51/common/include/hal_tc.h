#ifndef HAL_TC_H
#define HAL_TC_H

#include "sam.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * SAMD51 Timer/Counter (TC) HAL
 *
 * Supports TC0-TC7 in 16-bit mode for PWM and periodic interrupts.
 * Each TC instance must be clock-gated (MCLK) and have a GCLK source
 * routed before use.
 *
 * SAMD51 TC GCLK channel IDs:
 *   TC0/TC1: TC0_GCLK_ID (9)
 *   TC2/TC3: TC2_GCLK_ID (26)
 *   TC4/TC5: TC4_GCLK_ID (30)
 *   TC6/TC7: TC6_GCLK_ID (39)
 *
 * Instance count varies by package:
 *   100-pin (P20A): TC0-TC7 (8 instances)
 *   64-pin  (J20A): TC0-TC5 (6 instances)
 */

/* Derive instance count from DFP — TC7_REGS exists only on 100-pin packages */
#ifdef TC7_REGS
#define HAL_TC_INST_NUM 8
#else
#define HAL_TC_INST_NUM 6
#endif

/** TC operating mode */
typedef enum {
    HAL_TC_MODE_COUNT16,    /* 16-bit counter (periodic interrupt) */
    HAL_TC_MODE_NPWM,      /* Normal PWM — period in CC0, duty in CC1 */
} hal_tc_mode_t;

/** TC callback type */
typedef void (*hal_tc_callback_t)(void);

/**
 * Initialize a TC instance.
 *
 * @param tc        Pointer to TC registers (e.g., TC0_REGS)
 * @param mode      Operating mode
 * @param prescaler Prescaler value (0=DIV1, 1=DIV2, 2=DIV4, ... 7=DIV1024)
 */
void hal_tc_init(tc_registers_t *tc, hal_tc_mode_t mode, uint8_t prescaler);

/**
 * Set the period (TOP value) for 16-bit counter or NPWM mode.
 * In NPWM mode this sets CC0 (the period).
 */
void hal_tc_set_period(tc_registers_t *tc, uint16_t period);

/**
 * Set the compare/duty value.
 * In NPWM mode this sets CC1 (duty cycle count).
 */
void hal_tc_set_compare(tc_registers_t *tc, uint16_t compare);

/**
 * Set PWM duty cycle as a percentage (0-100).
 * Convenience wrapper: compare = period * duty / 100.
 */
void hal_tc_set_duty_percent(tc_registers_t *tc, uint8_t duty);

/** Start the timer. */
void hal_tc_start(tc_registers_t *tc);

/** Stop the timer. */
void hal_tc_stop(tc_registers_t *tc);

/**
 * Register an overflow interrupt callback.
 * Pass NULL to disable. The caller must enable the TC IRQ in NVIC.
 */
void hal_tc_set_overflow_callback(tc_registers_t *tc, hal_tc_callback_t cb);

/**
 * Call from the TC IRQHandler to dispatch callbacks.
 */
void hal_tc_irq_handler(tc_registers_t *tc);

#endif /* HAL_TC_H */
