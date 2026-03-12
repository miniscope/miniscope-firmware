#ifndef ENCODE_8B10B_H
#define ENCODE_8B10B_H

#include <stdint.h>

/**
 * 8b/10b line encoding.
 *
 * Pure computation (lookup tables) — no hardware dependency.
 * Used for encoded SPI output to achieve DC balance and
 * guaranteed transitions for clock recovery.
 *
 * Guarded by FW_ENABLE_8B10B.
 */

/** Running disparity state */
typedef struct {
    int8_t rd;  /* Running disparity: -1 or +1 */
} encode_8b10b_state_t;

/**
 * Initialize encoder state.
 * @param state  Encoder state to initialize
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

#endif /* ENCODE_8B10B_H */
