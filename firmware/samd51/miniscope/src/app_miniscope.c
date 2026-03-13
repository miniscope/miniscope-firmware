#include "app.h"
#include "board.h"
#include "miniscope_config.h"
#include "hal_systick.h"
#include "hal_gpio.h"
#include "hal_eic.h"
#include "hal_clock.h"
#include "sd_format.h"

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

#if defined(FW_ENABLE_IR_RECEIVER)
#include "ir_receiver.h"
#endif

#include "miniscope_state.h"
#include "miniscope_buffer.h"
#include "pcc_capture.h"

#include <string.h>

static miniscope_state_t s_state;
static buffer_pool_t s_pool;

/* Recording parameters (read from SD header block) */
static sd_header_block_t s_header;

/* Recording counters */
static uint32_t s_write_buffer_count;
static uint32_t s_record_start_ms;
static uint32_t s_sd_write_block;

/* EWL scan state */
static uint32_t s_ewl_scan_current;
static uint32_t s_ewl_scan_frame_count;

/* Trigger flags (set by ISR callbacks) */
static volatile bool s_ir_triggered;
static volatile bool s_button_triggered;

/* --- EIC Callbacks --- */

static void ir_trigger_callback(uint32_t command)
{
    (void)command;
    s_ir_triggered = true;
}

static void button_callback(uint8_t extint)
{
    (void)extint;
    s_button_triggered = true;
}

static void charge_callback(uint8_t extint)
{
    (void)extint;
    /* nCHRG status change — could track charging state */
}

/* --- EIC Setup --- */

static void eic_setup(void)
{
    /* Enable MCLK for EIC */
    hal_clock_enable_apb(&MCLK_REGS->MCLK_APBAMASK, MCLK_APBAMASK_EIC_Msk);

    /* Route GCLK0 to EIC for filtering */
    hal_clock_enable_gclk_channel(EIC_GCLK_ID, 0);

    /* EXTINT7 (PB23 nCHRG): BOTH edges, filter, no debounce */
    gpio_set_pmux(BOARD_NCHRG_PORT, BOARD_NCHRG_PIN, PORT_PMUX_PMUXO_A_Val);
    hal_eic_config_t nchrg_cfg = {
        .extint   = BOARD_NCHRG_EXTINT,
        .sense    = HAL_EIC_SENSE_BOTH,
        .filter   = true,
        .debounce = false,
    };
    hal_eic_configure_line(&nchrg_cfg);
    hal_eic_set_callback(BOARD_NCHRG_EXTINT, charge_callback);
    hal_eic_enable_line(BOARD_NCHRG_EXTINT);

    /* EXTINT9 (PA25 button): BOTH edges, filter, no debounce */
    gpio_set_pmux(BOARD_BUTTON_PORT, BOARD_BUTTON_PIN, PORT_PMUX_PMUXO_A_Val);
    hal_eic_config_t btn_cfg = {
        .extint   = BOARD_BUTTON_EXTINT,
        .sense    = HAL_EIC_SENSE_BOTH,
        .filter   = true,
        .debounce = false,
    };
    hal_eic_configure_line(&btn_cfg);
    hal_eic_set_callback(BOARD_BUTTON_EXTINT, button_callback);
    hal_eic_enable_line(BOARD_BUTTON_EXTINT);

    /* EXTINT14 (PB14 FrameValid): FALL edge, filter, no debounce */
    gpio_set_pmux(BOARD_FV_PORT, BOARD_FV_PIN, PORT_PMUX_PMUXE_A_Val);
    hal_eic_config_t fv_cfg = {
        .extint   = BOARD_FV_EXTINT,
        .sense    = HAL_EIC_SENSE_FALL,
        .filter   = true,
        .debounce = false,
    };
    hal_eic_configure_line(&fv_cfg);
    hal_eic_set_callback(BOARD_FV_EXTINT, pcc_frame_valid_callback);
    hal_eic_enable_line(BOARD_FV_EXTINT);
}

/* --- Recording Lifecycle --- */

static void start_recording(void)
{
    /* Reset counters */
    s_write_buffer_count = 0;
    s_record_start_ms = hal_systick_get_ticks();
    s_sd_write_block = MINISCOPE_SD_DATA_START;

    /* Read header block for recording parameters */
#if defined(FW_OUTPUT_SD)
    uint8_t block[SD_BLOCK_SIZE];
    if (sd_raw_read_block(MINISCOPE_SD_HEADER_BLOCK, block) == SD_OK) {
        sd_format_unpack_header(block, &s_header);
    }
#endif

    /* Configure sensor from header params */
#if defined(FW_SENSOR_PYTHON480)
    if (s_header.gain <= 2) {
        sensor_python480_set_gain((python480_gain_t)s_header.gain);
    }
    if (s_header.fps > 0) {
        sensor_python480_set_fps((uint8_t)s_header.fps);
    }
#endif

#if defined(FW_ENABLE_LED_PWM)
    if (s_header.led_power > 0) {
        led_control_set_duty((uint8_t)s_header.led_power);
        led_control_enable();
    }
#endif

#if defined(FW_ENABLE_EWL)
    ewl_set_voltage((uint8_t)s_header.ewl_value);
    s_ewl_scan_current = s_header.ewl_scan_start;
    s_ewl_scan_frame_count = 0;
#endif

    /* Status LED on during recording */
    gpio_set(BOARD_LED_PORT, BOARD_LED_PIN);

    /* Start capture — transitions to WAIT_FRAME_SYNC, then RECORDING
     * when the first frame-valid edge arrives */
    pcc_capture_start();
}

static void stop_recording(void)
{
    pcc_capture_stop();

    /* Write config block (block 1023) with final recording stats */
#if defined(FW_OUTPUT_SD)
    sd_config_block_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.width = MINISCOPE_WIDTH;
    cfg.height = MINISCOPE_HEIGHT;
    cfg.fps = s_header.fps;
    cfg.buffer_size = MINISCOPE_BUFFER_SIZE;
    cfg.buffers_recorded = s_write_buffer_count;
    cfg.buffers_dropped = pcc_capture_get_dropped_count();

    uint8_t block[SD_BLOCK_SIZE];
    sd_format_pack_config(block, &cfg);
    sd_raw_write_block(MINISCOPE_SD_CONFIG_BLOCK, block);
#endif

    /* Disable outputs */
#if defined(FW_ENABLE_LED_PWM)
    led_control_disable();
#endif

#if defined(FW_ENABLE_EWL)
    ewl_set_voltage(0);
#endif

    /* Status LED off */
    gpio_clear(BOARD_LED_PORT, BOARD_LED_PIN);
}

static void recording_loop(void)
{
    /* Drain filled buffers to SD */
#if defined(FW_OUTPUT_SD)
    while (buffer_pool_has_data(&s_pool)) {
        uint8_t idx = buffer_pool_read_index(&s_pool);
        uint8_t *buf = pcc_capture_get_buffer(idx);

        if (buf) {
            /* Update write-time fields in the buffer header */
            sd_buffer_meta_t meta;
            sd_format_read_meta(buf, &meta);
            meta.write_buffer_count = s_write_buffer_count;
            meta.write_timestamp_ms = hal_systick_get_ticks() - s_record_start_ms;

#if defined(FW_ENABLE_BATTERY_MONITOR)
            meta.battery_adc = battery_read_adc();
#endif

#if defined(FW_ENABLE_EWL)
            meta.ewl_value = s_ewl_scan_current;
#endif
            sd_format_write_meta(buf, &meta);

            /* Write buffer to SD (40 blocks per 20KB buffer) */
            uint32_t blocks = MINISCOPE_BUFFER_SIZE / SD_BLOCK_SIZE;
            sd_raw_write_multi(s_sd_write_block, buf, blocks);
            s_sd_write_block += blocks;
            s_write_buffer_count++;
        }

        buffer_pool_mark_consumed(&s_pool);
    }
#endif

    /* Check recording time limit */
    if (s_header.record_length > 0) {
        uint32_t elapsed_ms = hal_systick_get_ticks() - s_record_start_ms;
        if (elapsed_ms >= s_header.record_length * 1000) {
            miniscope_state_t evt_state = MINISCOPE_STATE_STOP_RECORDING;
            s_state = evt_state;
            return;
        }
    }

    /* EWL scan stepping */
#if defined(FW_ENABLE_EWL)
    if (s_header.ewl_scan_mode != 0 && s_header.ewl_scan_interval > 0) {
        uint32_t frame_num = pcc_capture_get_frame_num();
        if (frame_num > s_ewl_scan_frame_count + s_header.ewl_scan_interval) {
            s_ewl_scan_frame_count = frame_num;
            s_ewl_scan_current += s_header.ewl_scan_step;
            if (s_ewl_scan_current > s_header.ewl_scan_stop) {
                s_ewl_scan_current = s_header.ewl_scan_start;
            }
            ewl_set_voltage((uint8_t)s_ewl_scan_current);
        }
    }
#endif

#if defined(FW_ENABLE_BATTERY_MONITOR)
    if (battery_is_low()) {
        s_state = MINISCOPE_STATE_STOP_RECORDING;
    }
#endif
}

/* --- Public API --- */

void app_init(void)
{
#if defined(FW_OUTPUT_SD)
    sd_raw_init();
#endif

#if defined(FW_SENSOR_PYTHON480)
    sensor_python480_init();
    sensor_python480_enable_subsample();
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

    /* Setup EIC for all external interrupt lines */
    eic_setup();

#if defined(FW_ENABLE_IR_RECEIVER)
    ir_receiver_init();
    ir_receiver_set_callback(ir_trigger_callback);
#endif

    s_ir_triggered = false;
    s_button_triggered = false;
    memset(&s_header, 0, sizeof(s_header));

    /* Blink status LED once to signal successful init */
    gpio_set(BOARD_LED_PORT, BOARD_LED_PIN);
    hal_delay_ms(200);
    gpio_clear(BOARD_LED_PORT, BOARD_LED_PIN);
}

void app_run(void)
{
    miniscope_event_t event = MINISCOPE_EVENT_NONE;

    /* Check trigger sources */
    if (s_ir_triggered) {
        s_ir_triggered = false;
        event = MINISCOPE_EVENT_IR_COMMAND;
    }
    if (s_button_triggered) {
        s_button_triggered = false;
        event = MINISCOPE_EVENT_BUTTON_PRESS;
    }

#if defined(FW_ENABLE_BATTERY_MONITOR)
    if (battery_is_low()) {
        event = MINISCOPE_EVENT_BATTERY_LOW;
    }
#endif

    miniscope_state_t prev_state = s_state;
    miniscope_state_t new_state = miniscope_state_update(&s_state, event);

    /* Handle state transitions */
    if (new_state != prev_state) {
        switch (new_state) {
        case MINISCOPE_STATE_START_RECORDING:
            start_recording();
            /* Immediately transition to WAIT_FRAME_SYNC */
            s_state = MINISCOPE_STATE_WAIT_FRAME_SYNC;
            break;

        case MINISCOPE_STATE_STOP_RECORDING:
            stop_recording();
            s_state = MINISCOPE_STATE_IDLE;
            break;

        default:
            break;
        }
    }

    /* Ongoing state actions */
    switch (s_state) {
    case MINISCOPE_STATE_RECORDING:
        recording_loop();
        break;

    case MINISCOPE_STATE_WAIT_FRAME_SYNC:
        /* Frame-valid ISR will transition us to RECORDING */
        if (pcc_capture_get_frame_num() > 0) {
            s_state = MINISCOPE_STATE_RECORDING;
        }
        break;

    default:
        break;
    }
}
