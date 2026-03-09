#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include "sam.h"
#include <stdint.h>

/**
 * Minimal GPIO abstraction for SAMD51.
 * These are thin inline wrappers - zero overhead.
 *
 * Uses DFP v3.x register naming (PORT_REGS, PORT_DIRSET, etc.).
 */

/**
 * Set a pin as output.
 * @param port  Port index (0=A, 1=B, 2=C, 3=D).
 * @param pin   Pin number within port (0-31).
 */
static inline void gpio_set_output(uint8_t port, uint8_t pin)
{
    PORT_REGS->GROUP[port].PORT_DIRSET = (1u << pin);
}

/**
 * Drive a pin high.
 * @param port  Port index (0=A, 1=B, 2=C, 3=D).
 * @param pin   Pin number within port (0-31).
 */
static inline void gpio_set(uint8_t port, uint8_t pin)
{
    PORT_REGS->GROUP[port].PORT_OUTSET = (1u << pin);
}

/**
 * Drive a pin low.
 * @param port  Port index (0=A, 1=B, 2=C, 3=D).
 * @param pin   Pin number within port (0-31).
 */
static inline void gpio_clear(uint8_t port, uint8_t pin)
{
    PORT_REGS->GROUP[port].PORT_OUTCLR = (1u << pin);
}

/**
 * Toggle a pin output.
 * @param port  Port index (0=A, 1=B, 2=C, 3=D).
 * @param pin   Pin number within port (0-31).
 */
static inline void gpio_toggle(uint8_t port, uint8_t pin)
{
    PORT_REGS->GROUP[port].PORT_OUTTGL = (1u << pin);
}

/**
 * Read a pin input value.
 * @param port  Port index (0=A, 1=B, 2=C, 3=D).
 * @param pin   Pin number within port (0-31).
 * @return 1 if pin is high, 0 if low.
 */
static inline uint8_t gpio_read(uint8_t port, uint8_t pin)
{
    return (PORT_REGS->GROUP[port].PORT_IN >> pin) & 1u;
}

/**
 * Route a pin to a peripheral function via PMUX.
 *
 * pmux_func is the raw mux value (A=0, B=1, C=2, D=3, ...).
 * Use the DFP _Val defines: PORT_PMUX_PMUXE_A_Val through _N_Val.
 *
 * Example — route PB16 to SERCOM5/PAD0 (function C):
 *   gpio_set_pmux(1, 16, PORT_PMUX_PMUXE_C_Val);
 */
static inline void gpio_set_pmux(uint8_t port, uint8_t pin, uint8_t pmux_func)
{
    uint8_t pmux_idx = pin >> 1;
    if (pin & 1u) {
        PORT_REGS->GROUP[port].PORT_PMUX[pmux_idx] =
            (PORT_REGS->GROUP[port].PORT_PMUX[pmux_idx] & PORT_PMUX_PMUXE_Msk)
            | PORT_PMUX_PMUXO(pmux_func);
    } else {
        PORT_REGS->GROUP[port].PORT_PMUX[pmux_idx] =
            (PORT_REGS->GROUP[port].PORT_PMUX[pmux_idx] & PORT_PMUX_PMUXO_Msk)
            | PORT_PMUX_PMUXE(pmux_func);
    }
    PORT_REGS->GROUP[port].PORT_PINCFG[pin] |= PORT_PINCFG_PMUXEN_Msk;
}

/** Revert a pin from peripheral mux back to GPIO mode. */
static inline void gpio_clear_pmux(uint8_t port, uint8_t pin)
{
    PORT_REGS->GROUP[port].PORT_PINCFG[pin] &= ~PORT_PINCFG_PMUXEN_Msk;
}

#endif /* HAL_GPIO_H */
