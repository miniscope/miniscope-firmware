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
 *
 * Pin mux: PA16-PA23 (PCC DATA[0:7]), PA12 (DEN1), PA13 (DEN2),
 * PA14 (PCLK) — all routed to PMUX function K.
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

#endif /* PCC_CAPTURE_H */
