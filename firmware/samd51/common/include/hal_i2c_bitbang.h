#ifndef HAL_I2C_BITBANG_H
#define HAL_I2C_BITBANG_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Bit-bang I2C master driver.
 *
 * Pin-agnostic: takes port/pin for SDA and SCL.
 * Uses open-drain emulation: output register is always 0,
 * direction is toggled (OUT=low, IN=release/high via pull-up).
 *
 * Requires external pull-up resistors on both SDA and SCL.
 */

/** I2C bus instance */
typedef struct {
    uint8_t sda_port;
    uint8_t sda_pin;
    uint8_t scl_port;
    uint8_t scl_pin;
} hal_i2c_bb_t;

/**
 * Initialize the I2C bus pins.
 * Configures both pins as inputs (released high).
 */
void hal_i2c_bb_init(const hal_i2c_bb_t *bus);

/**
 * Write data to an I2C device.
 *
 * @param bus   I2C bus instance
 * @param addr  7-bit device address
 * @param data  Data buffer to write
 * @param len   Number of bytes to write
 * @return true on success (all bytes ACKed), false on NACK
 */
bool hal_i2c_bb_write(const hal_i2c_bb_t *bus,
                      uint8_t addr,
                      const uint8_t *data,
                      uint8_t len);

/**
 * Read data from an I2C device.
 *
 * @param bus   I2C bus instance
 * @param addr  7-bit device address
 * @param data  Buffer to read into
 * @param len   Number of bytes to read
 * @return true on success, false on NACK during address phase
 */
bool hal_i2c_bb_read(const hal_i2c_bb_t *bus,
                     uint8_t addr,
                     uint8_t *data,
                     uint8_t len);

#endif /* HAL_I2C_BITBANG_H */
