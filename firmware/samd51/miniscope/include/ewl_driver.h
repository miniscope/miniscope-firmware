#ifndef EWL_DRIVER_H
#define EWL_DRIVER_H

#include <stdint.h>

/**
 * Electrically-tunable Wetting Lens (EWL) driver.
 *
 * Controls an HV892 boost converter via bit-bang I2C.
 * The HV892 at address 0x23 sets voltage 0-255.
 *
 * Guarded by FW_ENABLE_EWL.
 */

/** Initialize the EWL I2C bus and driver. */
void ewl_init(void);

/**
 * Set the EWL voltage.
 * @param voltage  DAC value 0-255
 */
void ewl_set_voltage(uint8_t voltage);

/**
 * Run an EWL scan (sweep from start to stop with step).
 * @param start     Start voltage (0-255)
 * @param stop      Stop voltage (0-255)
 * @param step      Step size
 * @param interval_ms  Delay between steps in milliseconds
 */
void ewl_scan(uint8_t start, uint8_t stop, uint8_t step,
              uint32_t interval_ms);

#endif /* EWL_DRIVER_H */
