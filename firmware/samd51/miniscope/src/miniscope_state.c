#include "miniscope_state.h"

void miniscope_state_init(miniscope_state_t *state)
{
    *state = MINISCOPE_STATE_IDLE;
}

miniscope_state_t miniscope_state_update(miniscope_state_t *state,
                                         miniscope_event_t event)
{
    switch (*state) {
    case MINISCOPE_STATE_IDLE:
        if (event == MINISCOPE_EVENT_BUTTON_PRESS ||
            event == MINISCOPE_EVENT_IR_COMMAND) {
            *state = MINISCOPE_STATE_START_RECORDING;
        }
        break;

    case MINISCOPE_STATE_START_RECORDING:
        /* Transition immediately to wait for frame sync */
        *state = MINISCOPE_STATE_WAIT_FRAME_SYNC;
        break;

    case MINISCOPE_STATE_WAIT_FRAME_SYNC:
        if (event == MINISCOPE_EVENT_FRAME_SYNC) {
            *state = MINISCOPE_STATE_RECORDING;
        } else if (event == MINISCOPE_EVENT_TIMEOUT) {
            *state = MINISCOPE_STATE_STOP_RECORDING;
        }
        break;

    case MINISCOPE_STATE_RECORDING:
        if (event == MINISCOPE_EVENT_BUTTON_PRESS ||
            event == MINISCOPE_EVENT_IR_COMMAND ||
            event == MINISCOPE_EVENT_BATTERY_LOW ||
            event == MINISCOPE_EVENT_SD_FULL ||
            event == MINISCOPE_EVENT_RECORD_COMPLETE ||
            event == MINISCOPE_EVENT_TIMEOUT) {
            *state = MINISCOPE_STATE_STOP_RECORDING;
        }
        break;

    case MINISCOPE_STATE_STOP_RECORDING:
        /* Transition back to idle after cleanup */
        *state = MINISCOPE_STATE_IDLE;
        break;
    }

    return *state;
}
