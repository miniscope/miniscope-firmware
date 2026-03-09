#include "unity.h"
#include "app.h"
#include "blinky_config.h"
#include "hal_gpio.h"
#include "hal_systick.h"

#include <string.h>

void setUp(void)
{
    memset(mock_gpio_dir, 0, sizeof(mock_gpio_dir));
    memset(mock_gpio_out, 0, sizeof(mock_gpio_out));
    mock_delay_ms_total = 0;
}

void tearDown(void)
{
}

void test_app_run_toggles_led(void)
{
    /* LED starts off (0) */
    TEST_ASSERT_EQUAL_UINT32(0, mock_gpio_out[BLINKY_LED_PORT] & (1u << BLINKY_LED_PIN));

    /* First call: should toggle LED on */
    app_run();
    TEST_ASSERT_EQUAL_UINT32(1u << BLINKY_LED_PIN,
                             mock_gpio_out[BLINKY_LED_PORT] & (1u << BLINKY_LED_PIN));

    /* Second call: should toggle LED off */
    app_run();
    TEST_ASSERT_EQUAL_UINT32(0, mock_gpio_out[BLINKY_LED_PORT] & (1u << BLINKY_LED_PIN));
}

void test_app_run_delays(void)
{
    app_run();
    TEST_ASSERT_EQUAL_UINT32(BLINKY_DELAY_MS, mock_delay_ms_total);

    app_run();
    TEST_ASSERT_EQUAL_UINT32(BLINKY_DELAY_MS * 2, mock_delay_ms_total);
}

void test_blinky_config_defaults(void)
{
    TEST_ASSERT_EQUAL(0, BLINKY_LED_PORT);
    TEST_ASSERT_EQUAL(16, BLINKY_LED_PIN);
    TEST_ASSERT_EQUAL(500, BLINKY_DELAY_MS);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_app_run_toggles_led);
    RUN_TEST(test_app_run_delays);
    RUN_TEST(test_blinky_config_defaults);
    return UNITY_END();
}
