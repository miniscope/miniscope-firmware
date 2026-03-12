#ifndef HAL_SERCOM_UART_H
#define HAL_SERCOM_UART_H

#include "sam.h"
#include <stdint.h>

/**
 * SAMD51 SERCOM UART HAL — TX only (blocking).
 *
 * Provides basic serial output for debug logging and HIL test output.
 * Guarded by FW_ENABLE_UART feature flag.
 *
 * The caller must:
 *   1. Enable MCLK for the SERCOM instance
 *   2. Route a GCLK to the SERCOM's peripheral channel
 *   3. Configure the TX pin PMUX to the correct SERCOM pad
 */

/**
 * Initialize a SERCOM instance in UART mode (TX only).
 *
 * @param sercom    SERCOM registers (e.g., SERCOM5_REGS)
 * @param baud_hz   Desired baud rate
 * @param gclk_hz   GCLK frequency feeding this SERCOM
 * @param txpad     TX pad selection (0-3)
 */
void hal_uart_init(sercom_registers_t *sercom,
                   uint32_t baud_hz,
                   uint32_t gclk_hz,
                   uint8_t txpad);

/** Send a single character (blocking). */
void hal_uart_putc(sercom_registers_t *sercom, char c);

/** Send a null-terminated string (blocking). */
void hal_uart_puts(sercom_registers_t *sercom, const char *str);

/** Send a buffer of bytes (blocking). */
void hal_uart_write(sercom_registers_t *sercom,
                    const uint8_t *data,
                    uint32_t len);

#endif /* HAL_SERCOM_UART_H */
