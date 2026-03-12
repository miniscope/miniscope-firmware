#include "unity.h"
#include "encode_8b10b.h"

#include <string.h>

static encode_8b10b_state_t state;

void setUp(void)
{
    encode_8b10b_init(&state);
}

void tearDown(void) {}

void test_init_state(void)
{
    TEST_ASSERT_EQUAL_INT8(-1, state.rd);
}

void test_encode_byte_returns_10bit(void)
{
    uint16_t sym = encode_8b10b_byte(&state, 0x00);
    /* Result should fit in 10 bits */
    TEST_ASSERT_EQUAL_UINT16(0, sym & 0xFC00);
}

void test_k28_5_rd_minus(void)
{
    uint16_t sym = encode_8b10b_k28_5(&state);
    /* K28.5 RD- should be 0x17C (001111 1010) */
    TEST_ASSERT_EQUAL_HEX16(0x17C, sym);
    /* Disparity should have flipped to +1 */
    TEST_ASSERT_EQUAL_INT8(1, state.rd);
}

void test_k28_5_rd_plus(void)
{
    state.rd = 1;
    uint16_t sym = encode_8b10b_k28_5(&state);
    /* K28.5 RD+ should be 0x283 (110000 0101) */
    TEST_ASSERT_EQUAL_HEX16(0x283, sym);
    TEST_ASSERT_EQUAL_INT8(-1, state.rd);
}

void test_k28_5_alternates_disparity(void)
{
    int8_t initial_rd = state.rd;
    encode_8b10b_k28_5(&state);
    TEST_ASSERT_EQUAL_INT8(-initial_rd, state.rd);
    encode_8b10b_k28_5(&state);
    TEST_ASSERT_EQUAL_INT8(initial_rd, state.rd);
}

void test_buffer_encode(void)
{
    uint8_t input[4] = { 0x00, 0x01, 0x02, 0x03 };
    uint16_t output[4];

    encode_8b10b_buffer(&state, input, output, 4);

    /* Each output should be 10-bit */
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_UINT16(0, output[i] & 0xFC00);
    }
}

void test_disparity_bounded(void)
{
    /* Encode many bytes and verify RD stays at -1 or +1 */
    for (int i = 0; i < 256; i++) {
        encode_8b10b_byte(&state, (uint8_t)i);
        TEST_ASSERT_TRUE(state.rd == -1 || state.rd == 1);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_state);
    RUN_TEST(test_encode_byte_returns_10bit);
    RUN_TEST(test_k28_5_rd_minus);
    RUN_TEST(test_k28_5_rd_plus);
    RUN_TEST(test_k28_5_alternates_disparity);
    RUN_TEST(test_buffer_encode);
    RUN_TEST(test_disparity_bounded);
    return UNITY_END();
}
