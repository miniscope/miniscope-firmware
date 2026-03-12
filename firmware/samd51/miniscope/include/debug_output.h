#ifndef DEBUG_OUTPUT_H
#define DEBUG_OUTPUT_H

#include <stdint.h>

/**
 * Structured serial debug output for HIL testing and development.
 *
 * Output format:
 *   [BOOT] wirefree_v4 vX.Y.Z
 *   [INIT] clock=120MHz sd=OK sensor=OK
 *   [REC] start frame=0
 *   [REC] frame=100 dropped=0
 *   [STOP] total=1000 dropped=2
 *   [TEST] <name>=PASS|FAIL
 *
 * Requires FW_ENABLE_UART to be defined.
 */

/** Initialize debug UART output. */
void debug_init(void);

/** Print boot banner. */
void debug_boot(const char *board_name, const char *version);

/** Print initialization status. */
void debug_init_status(uint32_t clock_hz, int sd_ok, int sensor_ok);

/** Print recording start. */
void debug_rec_start(uint32_t frame);

/** Print recording progress. */
void debug_rec_progress(uint32_t frame, uint32_t dropped);

/** Print recording stop. */
void debug_rec_stop(uint32_t total_frames, uint32_t dropped);

/** Print test result. */
void debug_test_result(const char *test_name, int pass);

/** Print a raw string. */
void debug_puts(const char *str);

#endif /* DEBUG_OUTPUT_H */
