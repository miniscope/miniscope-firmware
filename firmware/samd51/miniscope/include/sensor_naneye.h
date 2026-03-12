#ifndef SENSOR_NANEYE_H
#define SENSOR_NANEYE_H

#include <stdint.h>

/**
 * NanEye image sensor driver over SERCOM SPI.
 *
 * Guarded by FW_SENSOR_NANEYE.
 */

/** Initialize the NanEye sensor. */
void sensor_naneye_init(void);

/** Start continuous frame output. */
void sensor_naneye_start(void);

/** Stop frame output. */
void sensor_naneye_stop(void);

#endif /* SENSOR_NANEYE_H */
