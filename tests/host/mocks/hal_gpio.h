#ifndef HAL_GPIO_H
#define HAL_GPIO_H

/**
 * Mock GPIO HAL for host-based testing.
 *
 * Provides observable state arrays so tests can verify GPIO operations
 * without real hardware.
 */

#include <stdint.h>

/* Maximum ports and pins for mock state tracking */
#define MOCK_GPIO_MAX_PORTS 4
#define MOCK_GPIO_MAX_PINS  32

/* Observable mock state */
extern uint32_t mock_gpio_dir[MOCK_GPIO_MAX_PORTS];
extern uint32_t mock_gpio_out[MOCK_GPIO_MAX_PORTS];

static inline void gpio_set_output(uint8_t port, uint8_t pin)
{
    if (port < MOCK_GPIO_MAX_PORTS) {
        mock_gpio_dir[port] |= (1u << pin);
    }
}

static inline void gpio_set(uint8_t port, uint8_t pin)
{
    if (port < MOCK_GPIO_MAX_PORTS) {
        mock_gpio_out[port] |= (1u << pin);
    }
}

static inline void gpio_clear(uint8_t port, uint8_t pin)
{
    if (port < MOCK_GPIO_MAX_PORTS) {
        mock_gpio_out[port] &= ~(1u << pin);
    }
}

static inline void gpio_toggle(uint8_t port, uint8_t pin)
{
    if (port < MOCK_GPIO_MAX_PORTS) {
        mock_gpio_out[port] ^= (1u << pin);
    }
}

static inline uint8_t gpio_read(uint8_t port, uint8_t pin)
{
    if (port < MOCK_GPIO_MAX_PORTS) {
        return (mock_gpio_out[port] >> pin) & 1u;
    }
    return 0;
}

#endif /* HAL_GPIO_H */
