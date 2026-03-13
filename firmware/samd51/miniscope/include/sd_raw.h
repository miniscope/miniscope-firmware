#ifndef SD_RAW_H
#define SD_RAW_H

#include <stdint.h>
#include <stdbool.h>

/**
 * SDHC0 register-level SD card driver.
 *
 * Provides raw block read/write over the SAMD51's SDHC0 peripheral.
 * Pin mux, clock gating, and GCLK routing are handled internally.
 *
 * Guarded by FW_OUTPUT_SD.
 */

/** SD card initialization result */
typedef enum {
    SD_OK = 0,
    SD_ERR_NO_CARD,
    SD_ERR_TIMEOUT,
    SD_ERR_CMD_FAIL,
    SD_ERR_CRC,
    SD_ERR_UNSUPPORTED,
} sd_error_t;

/**
 * Initialize SDHC0 and the SD card.
 * Configures pin mux, clocks, and runs the SD init sequence
 * (CMD0 → CMD8 → ACMD41 → CMD2 → CMD3 → CMD7 → CMD6 for 4-bit).
 *
 * @return SD_OK on success, error code on failure.
 */
sd_error_t sd_raw_init(void);

/**
 * Write a single 512-byte block.
 *
 * @param block_addr  Block address (LBA)
 * @param data        Pointer to 512 bytes of data
 * @return SD_OK on success
 */
sd_error_t sd_raw_write_block(uint32_t block_addr, const uint8_t *data);

/**
 * Write multiple contiguous 512-byte blocks.
 *
 * @param block_addr  Starting block address (LBA)
 * @param data        Pointer to data (block_count * 512 bytes)
 * @param block_count Number of blocks to write
 * @return SD_OK on success
 */
sd_error_t sd_raw_write_multi(uint32_t block_addr,
                              const uint8_t *data,
                              uint32_t block_count);

/**
 * Read a single 512-byte block.
 *
 * @param block_addr  Block address (LBA)
 * @param data        Buffer for 512 bytes of read data
 * @return SD_OK on success
 */
sd_error_t sd_raw_read_block(uint32_t block_addr, uint8_t *data);

/**
 * Check if an SD card is present.
 * @return true if card is inserted
 */
bool sd_raw_card_present(void);

#endif /* SD_RAW_H */
