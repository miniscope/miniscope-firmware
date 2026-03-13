#ifndef MINISCOPE_STATE_H
#define MINISCOPE_STATE_H

#include <stdint.h>

/**
 * Miniscope recording state machine.
 *
 * Pure logic — no hardware dependency. Testable on host.
 *
 * States: IDLE → START_RECORDING → WAIT_FRAME_SYNC → RECORDING
 *         → STOP_RECORDING → IDLE
 */

typedef enum {
    MINISCOPE_STATE_IDLE,
    MINISCOPE_STATE_START_RECORDING,
    MINISCOPE_STATE_WAIT_FRAME_SYNC,
    MINISCOPE_STATE_RECORDING,
    MINISCOPE_STATE_STOP_RECORDING,
} miniscope_state_t;

typedef enum {
    MINISCOPE_EVENT_NONE,
    MINISCOPE_EVENT_BUTTON_PRESS,
    MINISCOPE_EVENT_IR_COMMAND,
    MINISCOPE_EVENT_FRAME_SYNC,
    MINISCOPE_EVENT_TIMEOUT,
    MINISCOPE_EVENT_BATTERY_LOW,
    MINISCOPE_EVENT_SD_FULL,
    MINISCOPE_EVENT_RECORD_COMPLETE,
} miniscope_event_t;

/** Initialize the state machine to IDLE. */
void miniscope_state_init(miniscope_state_t *state);

/**
 * Process an event and transition state.
 *
 * @param state   Current state (updated in place)
 * @param event   Event to process
 * @return        New state after transition
 */
miniscope_state_t miniscope_state_update(miniscope_state_t *state,
                                         miniscope_event_t event);

#endif /* MINISCOPE_STATE_H */
