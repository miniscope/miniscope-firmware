#ifndef HAL_EIC_H
#define HAL_EIC_H

#include "sam.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * SAMD51 External Interrupt Controller (EIC) HAL
 *
 * The EIC provides 16 external interrupt lines (EXTINT[0..15]).
 * Each line maps to specific GPIO pins via the PMUX function A.
 * The EIC must be clock-gated (MCLK APB) and have a GCLK source
 * before use.
 *
 * GCLK channel: EIC_GCLK_ID (4)
 */

/** EIC sense mode */
typedef enum {
    HAL_EIC_SENSE_NONE    = 0,  /* No detection */
    HAL_EIC_SENSE_RISE    = 1,  /* Rising edge */
    HAL_EIC_SENSE_FALL    = 2,  /* Falling edge */
    HAL_EIC_SENSE_BOTH    = 3,  /* Both edges */
    HAL_EIC_SENSE_HIGH    = 4,  /* High level */
    HAL_EIC_SENSE_LOW     = 5,  /* Low level */
} hal_eic_sense_t;

/** EIC line configuration */
typedef struct {
    uint8_t         extint;     /* EXTINT line number (0-15) */
    hal_eic_sense_t sense;      /* Sense mode */
    bool            filter;     /* Enable input filter (requires GCLK) */
    bool            debounce;   /* Enable debounce */
} hal_eic_config_t;

/** EIC callback type */
typedef void (*hal_eic_callback_t)(uint8_t extint);

/**
 * Initialize the EIC (one-time).
 * Enables the EIC in the default clock configuration.
 */
void hal_eic_init(void);

/**
 * Configure an individual EXTINT line.
 * The GPIO pin must be routed to PMUX function A by the caller.
 */
void hal_eic_configure_line(const hal_eic_config_t *config);

/** Enable interrupt generation for an EXTINT line. */
void hal_eic_enable_line(uint8_t extint);

/** Disable interrupt generation for an EXTINT line. */
void hal_eic_disable_line(uint8_t extint);

/**
 * Register a callback for an EXTINT line.
 * Pass NULL to unregister.
 */
void hal_eic_set_callback(uint8_t extint, hal_eic_callback_t cb);

/**
 * Call from the EIC IRQHandler(s) to dispatch callbacks.
 * The SAMD51 has separate IRQ vectors for EXTINT[0..3] and one
 * shared for EXTINT[4..15].
 */
void hal_eic_irq_handler(void);

#endif /* HAL_EIC_H */
