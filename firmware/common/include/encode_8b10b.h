#ifndef ENCODE_8B10B_H
#define ENCODE_8B10B_H

#include <stdint.h>

/**
 * 8b/10b line encoding.
 *
 * Flat lookup-table implementation — single table lookup per byte
 * with branchless running-disparity tracking.
 *
 * Used for encoded SPI output to achieve DC balance and
 * guaranteed transitions for clock recovery.
 *
 * Guarded by FW_ENABLE_8B10B.
 */

/** Running disparity state */
typedef struct {
    uint8_t rd;  /* Running disparity: 0=RD-, 1=RD+ */
} encode_8b10b_state_t;

/**
 * Initialize encoder state.
 * @param state  Encoder state to initialize (sets RD-)
 */
void encode_8b10b_init(encode_8b10b_state_t *state);

/**
 * Encode a single byte to a 10-bit symbol.
 *
 * @param state  Encoder state (running disparity updated)
 * @param byte   Input byte to encode
 * @return       10-bit encoded symbol (in lower 10 bits of uint16_t)
 */
uint16_t encode_8b10b_byte(encode_8b10b_state_t *state, uint8_t byte);

/**
 * Encode a K28.5 comma symbol (for synchronization).
 *
 * @param state  Encoder state
 * @return       10-bit K28.5 symbol
 */
uint16_t encode_8b10b_k28_5(encode_8b10b_state_t *state);

/**
 * Encode a buffer of bytes.
 *
 * @param state   Encoder state
 * @param input   Input byte buffer
 * @param output  Output 16-bit buffer (10-bit symbols in lower bits)
 * @param count   Number of bytes to encode
 */
void encode_8b10b_buffer(encode_8b10b_state_t *state,
                         const uint8_t *input,
                         uint16_t *output,
                         uint32_t count);

/**
 * Pack 10-bit symbols into 32-bit words (LSB-first bit packing).
 *
 * @param symbols       Array of 10-bit symbols (in lower 10 bits)
 * @param packed        Output array of 32-bit packed words
 * @param symbol_count  Number of symbols to pack
 */
void encode_8b10b_pack_10to32(const uint16_t *symbols,
                               uint32_t *packed,
                               uint32_t symbol_count);

/**
 * Encode 16 bytes and pack into 5 x 32-bit words (160 bits).
 * Combined encode + pack for DMA-friendly output.
 *
 * @param state   Encoder state
 * @param input   16 input bytes
 * @param packed  5 output words (160 bits of packed 10-bit symbols)
 */
void encode_8b10b_encode_and_pack_16(encode_8b10b_state_t *state,
                                      const uint8_t input[16],
                                      uint32_t packed[5]);

#endif /* ENCODE_8B10B_H */
