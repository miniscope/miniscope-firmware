#include "unity.h"
#include "app.h"
#include "blinky_config.h"
#include "hal_gpio.h"
#include "hal_systick.h"
#include "hal_usb_cdc.h"

#include <string.h>

void setUp(void)
{
    memset(mock_gpio_dir, 0, sizeof(mock_gpio_dir));
    memset(mock_gpio_out, 0, sizeof(mock_gpio_out));
    mock_delay_ms_total = 0;
    mock_tick_value = 0;
    mock_usb_cdc_connected = false;
    memset(mock_usb_cdc_output, 0, sizeof(mock_usb_cdc_output));
    mock_usb_cdc_output_pos = 0;
    mock_usb_cdc_task_count = 0;
    app_init();
}

void tearDown(void)
{
}

/* -------------------------------------------------------------------
 * LED blink tests (non-blocking timing)
 * ------------------------------------------------------------------- */

void test_no_toggle_before_delay(void)
{
    mock_tick_value = BLINKY_DELAY_MS - 1;
    app_run();
    TEST_ASSERT_EQUAL_UINT32(0,
        mock_gpio_out[BLINKY_LED_PORT] & (1u << BLINKY_LED_PIN));
}

void test_toggles_after_delay(void)
{
    mock_tick_value = BLINKY_DELAY_MS;
    app_run();
    TEST_ASSERT_EQUAL_UINT32(1u << BLINKY_LED_PIN,
        mock_gpio_out[BLINKY_LED_PORT] & (1u << BLINKY_LED_PIN));
}

void test_toggles_back_after_two_delays(void)
{
    /* First toggle */
    mock_tick_value = BLINKY_DELAY_MS;
    app_run();
    TEST_ASSERT_EQUAL_UINT32(1u << BLINKY_LED_PIN,
        mock_gpio_out[BLINKY_LED_PORT] & (1u << BLINKY_LED_PIN));

    /* Second toggle */
    mock_tick_value = BLINKY_DELAY_MS * 2;
    app_run();
    TEST_ASSERT_EQUAL_UINT32(0,
        mock_gpio_out[BLINKY_LED_PORT] & (1u << BLINKY_LED_PIN));
}

/* -------------------------------------------------------------------
 * USB CDC task tests
 * ------------------------------------------------------------------- */

void test_cdc_task_called(void)
{
    app_run();
    TEST_ASSERT_GREATER_THAN(0, mock_usb_cdc_task_count);
}

/* -------------------------------------------------------------------
 * USB CDC message tests
 * ------------------------------------------------------------------- */

void test_boot_message_on_connect(void)
{
    mock_usb_cdc_connected = true;
    app_run();
    TEST_ASSERT_NOT_NULL(strstr(mock_usb_cdc_output, "[BOOT]"));
    TEST_ASSERT_NOT_NULL(strstr(mock_usb_cdc_output, "miniscope-fw"));
}

void test_no_boot_message_when_disconnected(void)
{
    mock_usb_cdc_connected = false;
    app_run();
    TEST_ASSERT_EQUAL_INT(0, mock_usb_cdc_output_pos);
}

void test_boot_message_sent_once(void)
{
    mock_usb_cdc_connected = true;
    app_run();
    int first_len = mock_usb_cdc_output_pos;
    TEST_ASSERT_GREATER_THAN(0, first_len);

    /* Clear output, call again — should NOT send boot again */
    memset(mock_usb_cdc_output, 0, sizeof(mock_usb_cdc_output));
    mock_usb_cdc_output_pos = 0;
    app_run();
    TEST_ASSERT_NULL(strstr(mock_usb_cdc_output, "[BOOT]"));
}

void test_heartbeat_at_1s_interval(void)
{
    mock_usb_cdc_connected = true;
    mock_tick_value = 1000;
    app_run();
    TEST_ASSERT_NOT_NULL(strstr(mock_usb_cdc_output, "[BLINK]"));
    TEST_ASSERT_NOT_NULL(strstr(mock_usb_cdc_output, "count="));
}

void test_no_heartbeat_before_interval(void)
{
    mock_usb_cdc_connected = true;
    mock_tick_value = 500;
    app_run();
    /* Boot message will be there, but no heartbeat */
    TEST_ASSERT_NULL(strstr(mock_usb_cdc_output, "[BLINK]"));
}

/* -------------------------------------------------------------------
 * Config defaults test
 * ------------------------------------------------------------------- */

void test_blinky_config_defaults(void)
{
    TEST_ASSERT_EQUAL(0, BLINKY_LED_PORT);
    TEST_ASSERT_EQUAL(16, BLINKY_LED_PIN);
    TEST_ASSERT_EQUAL(500, BLINKY_DELAY_MS);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_no_toggle_before_delay);
    RUN_TEST(test_toggles_after_delay);
    RUN_TEST(test_toggles_back_after_two_delays);
    RUN_TEST(test_cdc_task_called);
    RUN_TEST(test_boot_message_on_connect);
    RUN_TEST(test_no_boot_message_when_disconnected);
    RUN_TEST(test_boot_message_sent_once);
    RUN_TEST(test_heartbeat_at_1s_interval);
    RUN_TEST(test_no_heartbeat_before_interval);
    RUN_TEST(test_blinky_config_defaults);
    return UNITY_END();
}
