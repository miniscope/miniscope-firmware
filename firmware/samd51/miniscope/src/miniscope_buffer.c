#include "miniscope_buffer.h"

void buffer_pool_init(buffer_pool_t *pool)
{
    pool->write_idx     = 0;
    pool->read_idx      = 0;
    pool->pending_count = 0;
    pool->num_buffers   = MINISCOPE_NUM_BUFFERS;
    pool->total_filled  = 0;
    pool->total_consumed = 0;
    pool->dropped_count = 0;
}

bool buffer_pool_has_data(const buffer_pool_t *pool)
{
    return pool->pending_count > 0;
}

bool buffer_pool_is_full(const buffer_pool_t *pool)
{
    return pool->pending_count >= pool->num_buffers;
}

uint8_t buffer_pool_read_index(const buffer_pool_t *pool)
{
    return pool->read_idx;
}

uint8_t buffer_pool_write_index(const buffer_pool_t *pool)
{
    return pool->write_idx;
}

void buffer_pool_mark_filled(buffer_pool_t *pool)
{
    if (pool->pending_count >= pool->num_buffers) {
        /* Pool is full — drop this buffer */
        pool->dropped_count++;
        return;
    }

    pool->write_idx = (pool->write_idx + 1) % pool->num_buffers;
    pool->pending_count++;
    pool->total_filled++;
}

void buffer_pool_mark_consumed(buffer_pool_t *pool)
{
    if (pool->pending_count == 0) {
        return; /* Nothing to consume */
    }

    pool->read_idx = (pool->read_idx + 1) % pool->num_buffers;
    pool->pending_count--;
    pool->total_consumed++;
}

void buffer_pool_reset(buffer_pool_t *pool)
{
    pool->write_idx     = 0;
    pool->read_idx      = 0;
    pool->pending_count = 0;
    pool->total_filled  = 0;
    pool->total_consumed = 0;
    pool->dropped_count = 0;
}
