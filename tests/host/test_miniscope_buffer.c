#include "unity.h"
#include "miniscope_buffer.h"

#include <string.h>

static buffer_pool_t pool;

void setUp(void)
{
    buffer_pool_init(&pool);
}

void tearDown(void) {}

void test_init_state(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, pool.write_idx);
    TEST_ASSERT_EQUAL_UINT8(0, pool.read_idx);
    TEST_ASSERT_EQUAL_UINT8(0, pool.pending_count);
    TEST_ASSERT_EQUAL_UINT8(MINISCOPE_NUM_BUFFERS, pool.num_buffers);
    TEST_ASSERT_EQUAL_UINT32(0, pool.total_filled);
    TEST_ASSERT_EQUAL_UINT32(0, pool.total_consumed);
    TEST_ASSERT_EQUAL_UINT32(0, pool.dropped_count);
    TEST_ASSERT_FALSE(buffer_pool_has_data(&pool));
    TEST_ASSERT_FALSE(buffer_pool_is_full(&pool));
}

void test_fill_one_buffer(void)
{
    buffer_pool_mark_filled(&pool);

    TEST_ASSERT_TRUE(buffer_pool_has_data(&pool));
    TEST_ASSERT_EQUAL_UINT8(1, pool.pending_count);
    TEST_ASSERT_EQUAL_UINT8(1, pool.write_idx);
    TEST_ASSERT_EQUAL_UINT8(0, pool.read_idx);
    TEST_ASSERT_EQUAL_UINT32(1, pool.total_filled);
}

void test_fill_and_consume_one(void)
{
    buffer_pool_mark_filled(&pool);
    TEST_ASSERT_EQUAL_UINT8(0, buffer_pool_read_index(&pool));

    buffer_pool_mark_consumed(&pool);
    TEST_ASSERT_FALSE(buffer_pool_has_data(&pool));
    TEST_ASSERT_EQUAL_UINT8(0, pool.pending_count);
    TEST_ASSERT_EQUAL_UINT32(1, pool.total_consumed);
}

void test_fill_all_buffers(void)
{
    for (int i = 0; i < MINISCOPE_NUM_BUFFERS; i++) {
        buffer_pool_mark_filled(&pool);
    }

    TEST_ASSERT_TRUE(buffer_pool_is_full(&pool));
    TEST_ASSERT_EQUAL_UINT8(MINISCOPE_NUM_BUFFERS, pool.pending_count);
    TEST_ASSERT_EQUAL_UINT32(MINISCOPE_NUM_BUFFERS, pool.total_filled);
}

void test_overflow_drops(void)
{
    /* Fill all buffers */
    for (int i = 0; i < MINISCOPE_NUM_BUFFERS; i++) {
        buffer_pool_mark_filled(&pool);
    }

    /* Try to fill one more — should be dropped */
    buffer_pool_mark_filled(&pool);
    TEST_ASSERT_EQUAL_UINT32(1, pool.dropped_count);
    TEST_ASSERT_EQUAL_UINT8(MINISCOPE_NUM_BUFFERS, pool.pending_count);

    /* Multiple drops */
    buffer_pool_mark_filled(&pool);
    buffer_pool_mark_filled(&pool);
    TEST_ASSERT_EQUAL_UINT32(3, pool.dropped_count);
}

void test_consume_empty_is_noop(void)
{
    buffer_pool_mark_consumed(&pool);
    TEST_ASSERT_EQUAL_UINT8(0, pool.pending_count);
    TEST_ASSERT_EQUAL_UINT8(0, pool.read_idx);
    TEST_ASSERT_EQUAL_UINT32(0, pool.total_consumed);
}

void test_wraparound(void)
{
    /* Fill and consume all buffers twice to test wraparound */
    for (int round = 0; round < 2; round++) {
        for (int i = 0; i < MINISCOPE_NUM_BUFFERS; i++) {
            buffer_pool_mark_filled(&pool);
        }
        for (int i = 0; i < MINISCOPE_NUM_BUFFERS; i++) {
            TEST_ASSERT_TRUE(buffer_pool_has_data(&pool));
            buffer_pool_mark_consumed(&pool);
        }
        TEST_ASSERT_FALSE(buffer_pool_has_data(&pool));
    }

    TEST_ASSERT_EQUAL_UINT32(MINISCOPE_NUM_BUFFERS * 2, pool.total_filled);
    TEST_ASSERT_EQUAL_UINT32(MINISCOPE_NUM_BUFFERS * 2, pool.total_consumed);
    TEST_ASSERT_EQUAL_UINT32(0, pool.dropped_count);
}

void test_concurrent_fill_and_read(void)
{
    /* Simulate producer filling faster than consumer reading */
    buffer_pool_mark_filled(&pool);  /* 1 pending */
    buffer_pool_mark_filled(&pool);  /* 2 pending */
    buffer_pool_mark_consumed(&pool); /* 1 pending */
    buffer_pool_mark_filled(&pool);  /* 2 pending */
    buffer_pool_mark_consumed(&pool); /* 1 pending */
    buffer_pool_mark_consumed(&pool); /* 0 pending */

    TEST_ASSERT_FALSE(buffer_pool_has_data(&pool));
    TEST_ASSERT_EQUAL_UINT32(3, pool.total_filled);
    TEST_ASSERT_EQUAL_UINT32(3, pool.total_consumed);
    TEST_ASSERT_EQUAL_UINT32(0, pool.dropped_count);
}

void test_reset(void)
{
    buffer_pool_mark_filled(&pool);
    buffer_pool_mark_filled(&pool);
    buffer_pool_reset(&pool);

    TEST_ASSERT_EQUAL_UINT8(0, pool.write_idx);
    TEST_ASSERT_EQUAL_UINT8(0, pool.read_idx);
    TEST_ASSERT_EQUAL_UINT8(0, pool.pending_count);
    TEST_ASSERT_EQUAL_UINT32(0, pool.total_filled);
    TEST_ASSERT_EQUAL_UINT32(0, pool.total_consumed);
    TEST_ASSERT_EQUAL_UINT32(0, pool.dropped_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_state);
    RUN_TEST(test_fill_one_buffer);
    RUN_TEST(test_fill_and_consume_one);
    RUN_TEST(test_fill_all_buffers);
    RUN_TEST(test_overflow_drops);
    RUN_TEST(test_consume_empty_is_noop);
    RUN_TEST(test_wraparound);
    RUN_TEST(test_concurrent_fill_and_read);
    RUN_TEST(test_reset);
    return UNITY_END();
}
