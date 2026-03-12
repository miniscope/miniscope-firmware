#include "unity.h"
#include "miniscope_state.h"

static miniscope_state_t state;

void setUp(void)
{
    miniscope_state_init(&state);
}

void tearDown(void) {}

void test_init_is_idle(void)
{
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_IDLE, state);
}

void test_button_starts_recording(void)
{
    miniscope_state_update(&state, MINISCOPE_EVENT_BUTTON_PRESS);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_START_RECORDING, state);
}

void test_ir_starts_recording(void)
{
    miniscope_state_update(&state, MINISCOPE_EVENT_IR_COMMAND);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_START_RECORDING, state);
}

void test_start_transitions_to_wait_sync(void)
{
    miniscope_state_update(&state, MINISCOPE_EVENT_BUTTON_PRESS);
    miniscope_state_update(&state, MINISCOPE_EVENT_NONE);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_WAIT_FRAME_SYNC, state);
}

void test_frame_sync_starts_recording(void)
{
    miniscope_state_update(&state, MINISCOPE_EVENT_BUTTON_PRESS);
    miniscope_state_update(&state, MINISCOPE_EVENT_NONE); /* → WAIT_FRAME_SYNC */
    miniscope_state_update(&state, MINISCOPE_EVENT_FRAME_SYNC);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_RECORDING, state);
}

void test_button_stops_recording(void)
{
    /* Get to RECORDING state */
    miniscope_state_update(&state, MINISCOPE_EVENT_BUTTON_PRESS);
    miniscope_state_update(&state, MINISCOPE_EVENT_NONE);
    miniscope_state_update(&state, MINISCOPE_EVENT_FRAME_SYNC);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_RECORDING, state);

    /* Button press should stop recording */
    miniscope_state_update(&state, MINISCOPE_EVENT_BUTTON_PRESS);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_STOP_RECORDING, state);
}

void test_battery_low_stops_recording(void)
{
    /* Get to RECORDING state */
    miniscope_state_update(&state, MINISCOPE_EVENT_BUTTON_PRESS);
    miniscope_state_update(&state, MINISCOPE_EVENT_NONE);
    miniscope_state_update(&state, MINISCOPE_EVENT_FRAME_SYNC);

    miniscope_state_update(&state, MINISCOPE_EVENT_BATTERY_LOW);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_STOP_RECORDING, state);
}

void test_sd_full_stops_recording(void)
{
    /* Get to RECORDING state */
    miniscope_state_update(&state, MINISCOPE_EVENT_BUTTON_PRESS);
    miniscope_state_update(&state, MINISCOPE_EVENT_NONE);
    miniscope_state_update(&state, MINISCOPE_EVENT_FRAME_SYNC);

    miniscope_state_update(&state, MINISCOPE_EVENT_SD_FULL);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_STOP_RECORDING, state);
}

void test_timeout_during_wait_sync_stops(void)
{
    miniscope_state_update(&state, MINISCOPE_EVENT_BUTTON_PRESS);
    miniscope_state_update(&state, MINISCOPE_EVENT_NONE);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_WAIT_FRAME_SYNC, state);

    miniscope_state_update(&state, MINISCOPE_EVENT_TIMEOUT);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_STOP_RECORDING, state);
}

void test_stop_returns_to_idle(void)
{
    /* Get to STOP_RECORDING */
    miniscope_state_update(&state, MINISCOPE_EVENT_BUTTON_PRESS);
    miniscope_state_update(&state, MINISCOPE_EVENT_NONE);
    miniscope_state_update(&state, MINISCOPE_EVENT_FRAME_SYNC);
    miniscope_state_update(&state, MINISCOPE_EVENT_BUTTON_PRESS);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_STOP_RECORDING, state);

    /* Next update returns to idle */
    miniscope_state_update(&state, MINISCOPE_EVENT_NONE);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_IDLE, state);
}

void test_none_event_in_idle_stays_idle(void)
{
    miniscope_state_update(&state, MINISCOPE_EVENT_NONE);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_IDLE, state);
}

void test_invalid_event_in_idle_stays_idle(void)
{
    miniscope_state_update(&state, MINISCOPE_EVENT_FRAME_SYNC);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_IDLE, state);
}

void test_full_cycle(void)
{
    /* IDLE → START → WAIT → RECORDING → STOP → IDLE */
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_IDLE, state);

    miniscope_state_update(&state, MINISCOPE_EVENT_BUTTON_PRESS);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_START_RECORDING, state);

    miniscope_state_update(&state, MINISCOPE_EVENT_NONE);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_WAIT_FRAME_SYNC, state);

    miniscope_state_update(&state, MINISCOPE_EVENT_FRAME_SYNC);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_RECORDING, state);

    /* Record for a while (no stop events) */
    miniscope_state_update(&state, MINISCOPE_EVENT_NONE);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_RECORDING, state);

    miniscope_state_update(&state, MINISCOPE_EVENT_RECORD_COMPLETE);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_STOP_RECORDING, state);

    miniscope_state_update(&state, MINISCOPE_EVENT_NONE);
    TEST_ASSERT_EQUAL(MINISCOPE_STATE_IDLE, state);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_is_idle);
    RUN_TEST(test_button_starts_recording);
    RUN_TEST(test_ir_starts_recording);
    RUN_TEST(test_start_transitions_to_wait_sync);
    RUN_TEST(test_frame_sync_starts_recording);
    RUN_TEST(test_button_stops_recording);
    RUN_TEST(test_battery_low_stops_recording);
    RUN_TEST(test_sd_full_stops_recording);
    RUN_TEST(test_timeout_during_wait_sync_stops);
    RUN_TEST(test_stop_returns_to_idle);
    RUN_TEST(test_none_event_in_idle_stays_idle);
    RUN_TEST(test_invalid_event_in_idle_stays_idle);
    RUN_TEST(test_full_cycle);
    return UNITY_END();
}
