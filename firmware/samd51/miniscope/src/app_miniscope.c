#include "app.h"
#include "board.h"
#include "miniscope_config.h"
#include "hal_systick.h"
#include "hal_gpio.h"

#if defined(FW_SENSOR_PYTHON480)
#include "sensor_python480.h"
#endif

#if defined(FW_OUTPUT_SD)
#include "sd_raw.h"
#endif

#if defined(FW_ENABLE_LED_PWM)
#include "led_control.h"
#endif

#if defined(FW_ENABLE_EWL)
#include "ewl_driver.h"
#endif

#if defined(FW_ENABLE_BATTERY_MONITOR)
#include "battery_monitor.h"
#endif

#include "miniscope_state.h"
#include "miniscope_buffer.h"
#include "pcc_capture.h"

static miniscope_state_t s_state;
static buffer_pool_t s_pool;

void app_init(void)
{
#if defined(FW_OUTPUT_SD)
    sd_raw_init();
#endif

#if defined(FW_SENSOR_PYTHON480)
    sensor_python480_init();
#endif

#if defined(FW_ENABLE_LED_PWM)
    led_control_init();
#endif

#if defined(FW_ENABLE_EWL)
    ewl_init();
    ewl_set_voltage(MINISCOPE_EWL_DEFAULT);
#endif

#if defined(FW_ENABLE_BATTERY_MONITOR)
    battery_monitor_init();
#endif

    buffer_pool_init(&s_pool);
    pcc_capture_init(&s_pool);
    miniscope_state_init(&s_state);

    /* Blink status LED once to signal successful init */
    gpio_set(BOARD_LED_PORT, BOARD_LED_PIN);
    hal_delay_ms(200);
    gpio_clear(BOARD_LED_PORT, BOARD_LED_PIN);
}

void app_run(void)
{
    miniscope_event_t event = MINISCOPE_EVENT_NONE;

#if defined(FW_ENABLE_BATTERY_MONITOR)
    if (battery_is_low()) {
        event = MINISCOPE_EVENT_BATTERY_LOW;
    }
#endif

    miniscope_state_t new_state = miniscope_state_update(&s_state, event);
    (void)new_state;

    /* If recording, drain filled buffers to SD */
    if (s_state == MINISCOPE_STATE_RECORDING) {
#if defined(FW_OUTPUT_SD)
        while (buffer_pool_has_data(&s_pool)) {
            uint8_t idx = buffer_pool_read_index(&s_pool);
            /* sd_raw_write_block() would be called here with buffer data */
            (void)idx;
            buffer_pool_mark_consumed(&s_pool);
        }
#endif
    }
}
