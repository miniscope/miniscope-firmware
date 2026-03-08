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

static inline void gpio_set_output(uint8_t port, uint8_t pin)
{
    PORT_REGS->GROUP[port].PORT_DIRSET = (1u << pin);
}

static inline void gpio_set(uint8_t port, uint8_t pin)
{
    PORT_REGS->GROUP[port].PORT_OUTSET = (1u << pin);
}

static inline void gpio_clear(uint8_t port, uint8_t pin)
{
    PORT_REGS->GROUP[port].PORT_OUTCLR = (1u << pin);
}

static inline void gpio_toggle(uint8_t port, uint8_t pin)
{
    PORT_REGS->GROUP[port].PORT_OUTTGL = (1u << pin);
}

static inline uint8_t gpio_read(uint8_t port, uint8_t pin)
{
    return (PORT_REGS->GROUP[port].PORT_IN >> pin) & 1u;
}

#endif /* HAL_GPIO_H */
