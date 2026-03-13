#include "fw_config.h"
#include "debug_output.h"

#ifdef FW_ENABLE_UART

#include "hal_sercom_uart.h"
#include "hal_clock.h"
#include "board.h"
#include <stdio.h>

/*
 * Debug output via SERCOM UART.
 *
 * The specific SERCOM instance and TX pin depend on the board.
 * For now, this uses a compile-time selected SERCOM.
 * Boards should define BOARD_DEBUG_SERCOM, BOARD_DEBUG_TX_PORT/PIN,
 * BOARD_DEBUG_TX_PAD, BOARD_DEBUG_SERCOM_GCLK_ID.
 */

#ifndef BOARD_DEBUG_SERCOM
/* Default: no debug UART configured — all functions are stubs */
void debug_init(void) {}
void debug_boot(const char *board_name, const char *version) { (void)board_name; (void)version; }
void debug_init_status(uint32_t clock_hz, int sd_ok, int sensor_ok) { (void)clock_hz; (void)sd_ok; (void)sensor_ok; }
void debug_rec_start(uint32_t frame) { (void)frame; }
void debug_rec_progress(uint32_t frame, uint32_t dropped) { (void)frame; (void)dropped; }
void debug_rec_stop(uint32_t total_frames, uint32_t dropped) { (void)total_frames; (void)dropped; }
void debug_test_result(const char *test_name, int pass) { (void)test_name; (void)pass; }
void debug_puts(const char *str) { (void)str; }

#else

static char s_buf[128];

void debug_init(void)
{
    /* Route TX pin to SERCOM */
    gpio_set_pmux(BOARD_DEBUG_TX_PORT, BOARD_DEBUG_TX_PIN, PORT_PMUX_PMUXE_C_Val);

    /* Enable GCLK for SERCOM */
    hal_clock_enable_gclk_channel(BOARD_DEBUG_SERCOM_GCLK_ID, 0);

    /* Init UART at 115200 baud */
    hal_uart_init(BOARD_DEBUG_SERCOM, 115200,
                  hal_clock_get_cpu_freq(), BOARD_DEBUG_TX_PAD);
}

void debug_boot(const char *board_name, const char *version)
{
    snprintf(s_buf, sizeof(s_buf), "[BOOT] %s %s\r\n", board_name, version);
    hal_uart_puts(BOARD_DEBUG_SERCOM, s_buf);
}

void debug_init_status(uint32_t clock_hz, int sd_ok, int sensor_ok)
{
    snprintf(s_buf, sizeof(s_buf), "[INIT] clock=%luMHz sd=%s sensor=%s\r\n",
             clock_hz / 1000000UL,
             sd_ok ? "OK" : "FAIL",
             sensor_ok ? "OK" : "FAIL");
    hal_uart_puts(BOARD_DEBUG_SERCOM, s_buf);
}

void debug_rec_start(uint32_t frame)
{
    snprintf(s_buf, sizeof(s_buf), "[REC] start frame=%lu\r\n", frame);
    hal_uart_puts(BOARD_DEBUG_SERCOM, s_buf);
}

void debug_rec_progress(uint32_t frame, uint32_t dropped)
{
    snprintf(s_buf, sizeof(s_buf), "[REC] frame=%lu dropped=%lu\r\n",
             frame, dropped);
    hal_uart_puts(BOARD_DEBUG_SERCOM, s_buf);
}

void debug_rec_stop(uint32_t total_frames, uint32_t dropped)
{
    snprintf(s_buf, sizeof(s_buf), "[STOP] total=%lu dropped=%lu\r\n",
             total_frames, dropped);
    hal_uart_puts(BOARD_DEBUG_SERCOM, s_buf);
}

void debug_test_result(const char *test_name, int pass)
{
    snprintf(s_buf, sizeof(s_buf), "[TEST] %s=%s\r\n",
             test_name, pass ? "PASS" : "FAIL");
    hal_uart_puts(BOARD_DEBUG_SERCOM, s_buf);
}

void debug_puts(const char *str)
{
    hal_uart_puts(BOARD_DEBUG_SERCOM, str);
}

#endif /* BOARD_DEBUG_SERCOM */

#else /* !FW_ENABLE_UART */

/* No UART — all functions are stubs */
void debug_init(void) {}
void debug_boot(const char *board_name, const char *version) { (void)board_name; (void)version; }
void debug_init_status(uint32_t clock_hz, int sd_ok, int sensor_ok) { (void)clock_hz; (void)sd_ok; (void)sensor_ok; }
void debug_rec_start(uint32_t frame) { (void)frame; }
void debug_rec_progress(uint32_t frame, uint32_t dropped) { (void)frame; (void)dropped; }
void debug_rec_stop(uint32_t total_frames, uint32_t dropped) { (void)total_frames; (void)dropped; }
void debug_test_result(const char *test_name, int pass) { (void)test_name; (void)pass; }
void debug_puts(const char *str) { (void)str; }

#endif /* FW_ENABLE_UART */
