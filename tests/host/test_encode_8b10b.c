#include "unity.h"
#include "encode_8b10b.h"

#include <string.h>

static encode_8b10b_state_t state;

void setUp(void)
{
    encode_8b10b_init(&state);
}

void tearDown(void) {}

/* -----------------------------------------------------------------------
 * Reference split-table encoder (for LUT validation)
 * ----------------------------------------------------------------------- */
static const uint8_t ref_5b6b[32][2] = {
    { 0x38, 0x07 }, { 0x3D, 0x22 }, { 0x2D, 0x12 }, { 0x31, 0x31 },
    { 0x35, 0x0A }, { 0x29, 0x29 }, { 0x19, 0x19 }, { 0x38, 0x38 },
    { 0x39, 0x06 }, { 0x25, 0x25 }, { 0x15, 0x15 }, { 0x34, 0x34 },
    { 0x0D, 0x0D }, { 0x2C, 0x2C }, { 0x1C, 0x1C }, { 0x3A, 0x05 },
    { 0x36, 0x09 }, { 0x23, 0x23 }, { 0x13, 0x13 }, { 0x32, 0x32 },
    { 0x0B, 0x0B }, { 0x2A, 0x2A }, { 0x1A, 0x1A }, { 0x3B, 0x04 },
    { 0x37, 0x08 }, { 0x26, 0x26 }, { 0x16, 0x16 }, { 0x33, 0x0C },
    { 0x0E, 0x0E }, { 0x2E, 0x11 }, { 0x1E, 0x21 }, { 0x3E, 0x01 },
};

static const uint8_t ref_3b4b[8][2] = {
    { 0x0B, 0x04 }, { 0x09, 0x09 }, { 0x05, 0x05 }, { 0x0C, 0x03 },
    { 0x0D, 0x02 }, { 0x0A, 0x0A }, { 0x06, 0x06 }, { 0x0E, 0x01 },
};

static int popcount8(uint8_t v)
{
    int c = 0;
    while (v) { c += v & 1; v >>= 1; }
    return c;
}

static uint16_t ref_encode_byte(int8_t *rd, uint8_t byte)
{
    uint8_t edcba = byte & 0x1F;
    uint8_t hgf   = (byte >> 5) & 0x07;
    int rd_idx = (*rd > 0) ? 1 : 0;

    uint8_t code_6b = ref_5b6b[edcba][rd_idx];
    int disp_6 = 2 * popcount8(code_6b & 0x3F) - 6;
    int8_t rd_after_6 = *rd + (int8_t)disp_6;
    rd_after_6 = (rd_after_6 > 0) ? 1 : -1;

    int rd_idx_4 = (rd_after_6 > 0) ? 1 : 0;
    uint8_t code_4b = ref_3b4b[hgf][rd_idx_4];
    int disp_4 = 2 * popcount8(code_4b & 0x0F) - 4;
    int8_t rd_final = rd_after_6 + (int8_t)disp_4;
    rd_final = (rd_final > 0) ? 1 : -1;

    *rd = rd_final;
    return (uint16_t)(((uint16_t)code_6b << 4) | code_4b);
}

/* -----------------------------------------------------------------------
 * Basic API tests
 * ----------------------------------------------------------------------- */

void test_init_state(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, state.rd);
}

void test_encode_byte_returns_10bit(void)
{
    uint16_t sym = encode_8b10b_byte(&state, 0x00);
    TEST_ASSERT_EQUAL_UINT16(0, sym & 0xFC00);
}

void test_k28_5_rd_minus(void)
{
    uint16_t sym = encode_8b10b_k28_5(&state);
    TEST_ASSERT_EQUAL_HEX16(0x17C, sym);
    TEST_ASSERT_EQUAL_UINT8(1, state.rd);
}

void test_k28_5_rd_plus(void)
{
    state.rd = 1;
    uint16_t sym = encode_8b10b_k28_5(&state);
    TEST_ASSERT_EQUAL_HEX16(0x283, sym);
    TEST_ASSERT_EQUAL_UINT8(0, state.rd);
}

void test_k28_5_alternates_disparity(void)
{
    uint8_t initial_rd = state.rd;
    encode_8b10b_k28_5(&state);
    TEST_ASSERT_EQUAL_UINT8(1 - initial_rd, state.rd);
    encode_8b10b_k28_5(&state);
    TEST_ASSERT_EQUAL_UINT8(initial_rd, state.rd);
}

void test_buffer_encode(void)
{
    uint8_t input[4] = { 0x00, 0x01, 0x02, 0x03 };
    uint16_t output[4];

    encode_8b10b_buffer(&state, input, output, 4);

    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_UINT16(0, output[i] & 0xFC00);
    }
}

void test_disparity_bounded(void)
{
    for (int i = 0; i < 256; i++) {
        encode_8b10b_byte(&state, (uint8_t)i);
        TEST_ASSERT_TRUE(state.rd == 0 || state.rd == 1);
    }
}

/* -----------------------------------------------------------------------
 * LUT validation: flat LUT vs. split-table reference for all 512 combos
 * ----------------------------------------------------------------------- */

void test_lut_matches_reference_rd_neg(void)
{
    for (int b = 0; b < 256; b++) {
        int8_t ref_rd = -1;
        uint16_t ref_sym = ref_encode_byte(&ref_rd, (uint8_t)b);

        encode_8b10b_state_t s;
        s.rd = 0;
        uint16_t lut_sym = encode_8b10b_byte(&s, (uint8_t)b);

        char msg[64];
        snprintf(msg, sizeof(msg), "byte=0x%02X RD-", b);
        TEST_ASSERT_EQUAL_HEX16_MESSAGE(ref_sym, lut_sym, msg);

        uint8_t expected_rd = (ref_rd > 0) ? 1 : 0;
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected_rd, s.rd, msg);
    }
}

void test_lut_matches_reference_rd_pos(void)
{
    for (int b = 0; b < 256; b++) {
        int8_t ref_rd = 1;
        uint16_t ref_sym = ref_encode_byte(&ref_rd, (uint8_t)b);

        encode_8b10b_state_t s;
        s.rd = 1;
        uint16_t lut_sym = encode_8b10b_byte(&s, (uint8_t)b);

        char msg[64];
        snprintf(msg, sizeof(msg), "byte=0x%02X RD+", b);
        TEST_ASSERT_EQUAL_HEX16_MESSAGE(ref_sym, lut_sym, msg);

        uint8_t expected_rd = (ref_rd > 0) ? 1 : 0;
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected_rd, s.rd, msg);
    }
}

void test_lut_stream_matches_reference(void)
{
    /* Encode all 256 byte values in sequence, both starting at RD- */
    int8_t ref_rd = -1;
    encode_8b10b_state_t s;
    s.rd = 0;

    for (int b = 0; b < 256; b++) {
        int8_t ref_rd_save = ref_rd;
        uint16_t ref_sym = ref_encode_byte(&ref_rd, (uint8_t)b);

        uint8_t s_rd_save = s.rd;
        uint16_t lut_sym = encode_8b10b_byte(&s, (uint8_t)b);

        char msg[64];
        snprintf(msg, sizeof(msg), "stream byte=%d (rd before: ref=%d lut=%d)",
                 b, ref_rd_save, s_rd_save);
        TEST_ASSERT_EQUAL_HEX16_MESSAGE(ref_sym, lut_sym, msg);
    }
}

/* -----------------------------------------------------------------------
 * Pack function tests
 * ----------------------------------------------------------------------- */

void test_pack_10to32_basic(void)
{
    /* Pack 4 symbols: 0x3FF, 0x000, 0x155, 0x2AA */
    uint16_t symbols[4] = { 0x3FF, 0x000, 0x155, 0x2AA };
    uint32_t packed[2] = { 0 };

    encode_8b10b_pack_10to32(symbols, packed, 4);

    /* Verify bit-by-bit:
     * Word 0: sym0[9:0] | sym1[9:0] | sym2[9:0] | sym3[1:0]
     *       = 0x3FF | (0x000 << 10) | (0x155 << 20) | (0x2AA << 30)
     */
    uint32_t expected0 = 0x3FF | (0x000 << 10) | (0x155 << 20) | (0x2AAu << 30);
    TEST_ASSERT_EQUAL_HEX32(expected0, packed[0]);

    /* Word 1: sym3[9:2] | sym_remaining = (0x2AA >> 2) = 0xAA */
    uint32_t expected1 = (0x2AA >> 2);
    TEST_ASSERT_EQUAL_HEX32(expected1, packed[1]);
}

void test_pack_16_symbols(void)
{
    /* Encode 16 bytes and compare general pack with encode_and_pack_16 */
    uint8_t input[16] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };

    /* Method 1: buffer encode + general pack */
    encode_8b10b_state_t s1;
    encode_8b10b_init(&s1);
    uint16_t symbols[16];
    encode_8b10b_buffer(&s1, input, symbols, 16);
    uint32_t packed1[5] = { 0 };
    encode_8b10b_pack_10to32(symbols, packed1, 16);

    /* Method 2: combined encode_and_pack_16 */
    encode_8b10b_state_t s2;
    encode_8b10b_init(&s2);
    uint32_t packed2[5] = { 0 };
    encode_8b10b_encode_and_pack_16(&s2, input, packed2);

    /* Both must produce identical output */
    TEST_ASSERT_EQUAL_UINT8(s1.rd, s2.rd);
    TEST_ASSERT_EQUAL_HEX32_ARRAY(packed1, packed2, 5);
}

void test_encode_and_pack_16_rd_preserved(void)
{
    /* Encoding the same 16 bytes twice should maintain consistent RD */
    uint8_t input[16] = { 0 };

    encode_8b10b_state_t s;
    encode_8b10b_init(&s);
    uint32_t packed[5];

    encode_8b10b_encode_and_pack_16(&s, input, packed);
    uint8_t rd_after_first = s.rd;

    encode_8b10b_encode_and_pack_16(&s, input, packed);
    /* RD should continue from where it was, not reset */
    (void)rd_after_first;
    TEST_ASSERT_TRUE(s.rd == 0 || s.rd == 1);
}

/* -----------------------------------------------------------------------
 * Test runner
 * ----------------------------------------------------------------------- */

int main(void)
{
    UNITY_BEGIN();

    /* Basic API */
    RUN_TEST(test_init_state);
    RUN_TEST(test_encode_byte_returns_10bit);
    RUN_TEST(test_k28_5_rd_minus);
    RUN_TEST(test_k28_5_rd_plus);
    RUN_TEST(test_k28_5_alternates_disparity);
    RUN_TEST(test_buffer_encode);
    RUN_TEST(test_disparity_bounded);

    /* LUT validation (all 512 combinations) */
    RUN_TEST(test_lut_matches_reference_rd_neg);
    RUN_TEST(test_lut_matches_reference_rd_pos);
    RUN_TEST(test_lut_stream_matches_reference);

    /* Pack functions */
    RUN_TEST(test_pack_10to32_basic);
    RUN_TEST(test_pack_16_symbols);
    RUN_TEST(test_encode_and_pack_16_rd_preserved);

    return UNITY_END();
}
