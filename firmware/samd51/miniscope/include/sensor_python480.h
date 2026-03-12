#ifndef SENSOR_PYTHON480_H
#define SENSOR_PYTHON480_H

#include <stdint.h>

/**
 * ON Semiconductor Python480 image sensor driver.
 *
 * Uses bit-bang SPI for register access (9-bit address + R/W + 16-bit data).
 * Pin assignments come from board.h.
 *
 * Guarded by FW_SENSOR_PYTHON480.
 */

/** Sensor gain settings */
typedef enum {
    PYTHON480_GAIN_1X   = 0,
    PYTHON480_GAIN_2X   = 1,
    PYTHON480_GAIN_3P5X = 2,
} python480_gain_t;

/** Initialize the Python480 sensor.
 *  Runs the full power-on and register configuration sequence. */
void sensor_python480_init(void);

/** Set analog gain. */
void sensor_python480_set_gain(python480_gain_t gain);

/** Set target frame rate (5, 10, 15, or 20 FPS). */
void sensor_python480_set_fps(uint8_t fps);

/** Set exposure time in microseconds. */
void sensor_python480_set_exposure(uint32_t exposure_us);

/** Enable 2x2 subsampling (matching reference Enable_Subsample). */
void sensor_python480_enable_subsample(void);

/** Write a single register via bit-bang SPI. */
void sensor_python480_write_reg(uint16_t addr, uint16_t value);

/** Read a single register via bit-bang SPI. */
uint16_t sensor_python480_read_reg(uint16_t addr);

#endif /* SENSOR_PYTHON480_H */
