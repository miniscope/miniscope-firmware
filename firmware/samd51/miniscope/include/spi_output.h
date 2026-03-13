#ifndef SPI_OUTPUT_H
#define SPI_OUTPUT_H

#include <stdint.h>

/**
 * SPI MOSI encoded output driver.
 *
 * Sends captured frame data out via SERCOM SPI, optionally
 * with 8b/10b encoding for DC-balanced transmission.
 *
 * Guarded by FW_OUTPUT_SPI.
 */

/** Initialize the SPI output (SERCOM configuration). */
void spi_output_init(void);

/**
 * Send a buffer of data via SPI.
 *
 * @param data  Data buffer
 * @param len   Number of bytes to send
 */
void spi_output_send(const uint8_t *data, uint32_t len);

#endif /* SPI_OUTPUT_H */
