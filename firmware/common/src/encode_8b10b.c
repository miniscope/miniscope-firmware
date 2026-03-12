#include "encode_8b10b.h"

/*
 * 8b/10b encoding lookup tables.
 *
 * The 8b/10b code splits each byte into a 5-bit (EDCBA) and 3-bit (HGF)
 * portion, encoding them separately as 6b and 4b codes, then concatenating
 * to form the 10-bit output symbol.
 *
 * Each table entry stores two variants: one for RD- and one for RD+.
 * The disparity of each code determines whether the running disparity flips.
 *
 * Reference: IBM 8b/10b encoding standard (A.X. Widmer, P.A. Franaszek).
 */

/* 5b/6b encoding table: index = EDCBA (0-31)
 * Each entry: [RD- code, RD+ code]
 * 6-bit codes stored in lower 6 bits */
static const uint8_t enc_5b6b[32][2] = {
    { 0x38, 0x07 }, /* D.00 */
    { 0x3D, 0x22 }, /* D.01 */
    { 0x2D, 0x12 }, /* D.02 */
    { 0x31, 0x31 }, /* D.03 */
    { 0x35, 0x0A }, /* D.04 */
    { 0x29, 0x29 }, /* D.05 */
    { 0x19, 0x19 }, /* D.06 */
    { 0x38, 0x38 }, /* D.07 */
    { 0x39, 0x06 }, /* D.08 */
    { 0x25, 0x25 }, /* D.09 */
    { 0x15, 0x15 }, /* D.10 */
    { 0x34, 0x34 }, /* D.11 */
    { 0x0D, 0x0D }, /* D.12 */
    { 0x2C, 0x2C }, /* D.13 */
    { 0x1C, 0x1C }, /* D.14 */
    { 0x3A, 0x05 }, /* D.15 */
    { 0x36, 0x09 }, /* D.16 */
    { 0x23, 0x23 }, /* D.17 */
    { 0x13, 0x13 }, /* D.18 */
    { 0x32, 0x32 }, /* D.19 */
    { 0x0B, 0x0B }, /* D.20 */
    { 0x2A, 0x2A }, /* D.21 */
    { 0x1A, 0x1A }, /* D.22 */
    { 0x3B, 0x04 }, /* D.23 */
    { 0x37, 0x08 }, /* D.24 */
    { 0x26, 0x26 }, /* D.25 */
    { 0x16, 0x16 }, /* D.26 */
    { 0x33, 0x0C }, /* D.27 */
    { 0x0E, 0x0E }, /* D.28 */
    { 0x2E, 0x11 }, /* D.29 */
    { 0x1E, 0x21 }, /* D.30 */
    { 0x3E, 0x01 }, /* D.31 */
};

/* 3b/4b encoding table: index = HGF (0-7)
 * Each entry: [RD- code, RD+ code]
 * 4-bit codes stored in lower 4 bits */
static const uint8_t enc_3b4b[8][2] = {
    { 0x0B, 0x04 }, /* D.x.0 */
    { 0x09, 0x09 }, /* D.x.1 */
    { 0x05, 0x05 }, /* D.x.2 */
    { 0x0C, 0x03 }, /* D.x.3 */
    { 0x0D, 0x02 }, /* D.x.4 */
    { 0x0A, 0x0A }, /* D.x.5 */
    { 0x06, 0x06 }, /* D.x.6 */
    { 0x0E, 0x01 }, /* D.x.7 (primary) */
};

/* Count ones in 6-bit value */
static int count_ones_6(uint8_t val)
{
    int count = 0;
    for (int i = 0; i < 6; i++) {
        if (val & (1 << i)) count++;
    }
    return count;
}

/* Count ones in 4-bit value */
static int count_ones_4(uint8_t val)
{
    int count = 0;
    for (int i = 0; i < 4; i++) {
        if (val & (1 << i)) count++;
    }
    return count;
}

void encode_8b10b_init(encode_8b10b_state_t *state)
{
    state->rd = -1; /* Start with RD- */
}

uint16_t encode_8b10b_byte(encode_8b10b_state_t *state, uint8_t byte)
{
    uint8_t edcba = byte & 0x1F;        /* Lower 5 bits */
    uint8_t hgf   = (byte >> 5) & 0x07; /* Upper 3 bits */

    int rd_idx = (state->rd > 0) ? 1 : 0;

    /* 5b/6b encode */
    uint8_t code_6b = enc_5b6b[edcba][rd_idx];
    int ones_6 = count_ones_6(code_6b);
    int disparity_6 = ones_6 - (6 - ones_6); /* +/- disparity */

    /* Update running disparity after 6b code */
    int8_t rd_after_6 = state->rd + (int8_t)disparity_6;
    /* Clamp to -1 or +1 */
    if (rd_after_6 > 0) rd_after_6 = 1;
    else rd_after_6 = -1;

    /* 3b/4b encode with updated disparity */
    int rd_idx_4 = (rd_after_6 > 0) ? 1 : 0;
    uint8_t code_4b = enc_3b4b[hgf][rd_idx_4];
    int ones_4 = count_ones_4(code_4b);
    int disparity_4 = ones_4 - (4 - ones_4);

    /* Update final running disparity */
    int8_t rd_final = rd_after_6 + (int8_t)disparity_4;
    if (rd_final > 0) rd_final = 1;
    else rd_final = -1;

    state->rd = rd_final;

    /* Combine: 6b in upper bits, 4b in lower bits */
    return (uint16_t)(((uint16_t)code_6b << 4) | code_4b);
}

uint16_t encode_8b10b_k28_5(encode_8b10b_state_t *state)
{
    /* K28.5 special symbol */
    uint16_t symbol;
    if (state->rd < 0) {
        symbol = 0x17C; /* K28.5 RD- = 001111 1010 */
    } else {
        symbol = 0x283; /* K28.5 RD+ = 110000 0101 */
    }
    state->rd = -state->rd; /* K28.5 always flips disparity */
    return symbol;
}

void encode_8b10b_buffer(encode_8b10b_state_t *state,
                         const uint8_t *input,
                         uint16_t *output,
                         uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        output[i] = encode_8b10b_byte(state, input[i]);
    }
}
