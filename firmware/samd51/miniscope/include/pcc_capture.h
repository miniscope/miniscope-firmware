#ifndef PCC_CAPTURE_H
#define PCC_CAPTURE_H

#include <stdint.h>
#include <stdbool.h>
#include "miniscope_buffer.h"

/**
 * PCC + DMA capture engine.
 *
 * Configures the Parallel Capture Controller for 8-bit input with
 * 4-byte packing, driven by a DMA channel into circular RAM buffers.
 * CID=3 requires both DEN1 and DEN2 (matching reference firmware).
 *
 * Pin mux: PA16-PA23 (PCC DATA[0:7]), PA12 (DEN1), PA13 (DEN2),
 * PA14 (PCLK) — all routed to PMUX function K.
 *
 * DMA writes start 48 bytes (12 words) into each buffer, leaving
 * room for the per-buffer metadata header.
 */

/**
 * Initialize PCC and DMA for parallel capture.
 * Sets up pin mux, clocks, PCC registers, DMA descriptors.
 *
 * @param pool  Buffer pool for DMA target management
 */
void pcc_capture_init(buffer_pool_t *pool);

/** Start capturing data from the sensor. */
void pcc_capture_start(void);

/** Stop capturing. */
void pcc_capture_stop(void);

/** Check if capture is currently active. */
bool pcc_capture_is_active(void);

/** Get the global buffer count since capture started. */
uint32_t pcc_capture_get_buffer_count(void);

/** Get the current frame number. */
uint32_t pcc_capture_get_frame_num(void);

/** Get the number of dropped buffers. */
uint32_t pcc_capture_get_dropped_count(void);

/** Get pointer to a specific capture buffer. */
uint8_t *pcc_capture_get_buffer(uint8_t idx);

/**
 * Frame-valid falling-edge callback.
 * Register this with hal_eic_set_callback() for EXTINT14 (PB14).
 * Handles partial buffer at frame boundaries.
 */
void pcc_frame_valid_callback(uint8_t extint);

#endif /* PCC_CAPTURE_H */
