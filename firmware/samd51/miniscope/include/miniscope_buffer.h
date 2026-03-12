#ifndef MINISCOPE_BUFFER_H
#define MINISCOPE_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include "miniscope_config.h"

/**
 * Producer-consumer buffer pool for DMA capture data.
 *
 * Pure index arithmetic — no hardware dependency. The actual RAM
 * buffer allocation and DMA descriptor wiring happens in pcc_capture.c.
 *
 * Thread-safety model:
 *   - Producer (DMA ISR): calls mark_filled()
 *   - Consumer (main loop): calls mark_consumed()
 *   - Single producer, single consumer — no locks needed.
 */

typedef struct {
    uint8_t  write_idx;         /* Next buffer for DMA to fill */
    uint8_t  read_idx;          /* Next buffer for consumer to process */
    uint8_t  pending_count;     /* Number of filled buffers waiting */
    uint8_t  num_buffers;       /* Total buffers in pool */
    uint32_t total_filled;      /* Cumulative buffers filled */
    uint32_t total_consumed;    /* Cumulative buffers consumed */
    uint32_t dropped_count;     /* Buffers dropped due to overflow */
} buffer_pool_t;

/** Initialize buffer pool state. */
void buffer_pool_init(buffer_pool_t *pool);

/** Check if there are filled buffers ready to consume. */
bool buffer_pool_has_data(const buffer_pool_t *pool);

/** Check if the pool is full (would drop on next fill). */
bool buffer_pool_is_full(const buffer_pool_t *pool);

/** Get the current read index (for accessing the buffer data). */
uint8_t buffer_pool_read_index(const buffer_pool_t *pool);

/** Get the current write index (for DMA descriptor targeting). */
uint8_t buffer_pool_write_index(const buffer_pool_t *pool);

/**
 * Mark the current write buffer as filled (called by DMA ISR).
 * Advances write_idx. If pool is full, increments dropped_count.
 */
void buffer_pool_mark_filled(buffer_pool_t *pool);

/**
 * Mark the current read buffer as consumed (called by main loop).
 * Advances read_idx. No-op if no data pending.
 */
void buffer_pool_mark_consumed(buffer_pool_t *pool);

/** Reset the pool to empty state. */
void buffer_pool_reset(buffer_pool_t *pool);

#endif /* MINISCOPE_BUFFER_H */
