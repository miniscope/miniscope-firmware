#ifndef IR_RECEIVER_H
#define IR_RECEIVER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * IR remote receiver via EIC on PB22 (EXTINT[6]).
 *
 * Decodes IR signals for remote triggering of recording.
 * The specific protocol (NEC or similar) depends on the
 * remote control used with the Miniscope system.
 *
 * Guarded by FW_ENABLE_IR_RECEIVER.
 */

/** IR command callback type */
typedef void (*ir_callback_t)(uint32_t command);

/** Initialize the IR receiver (EIC on PB22). */
void ir_receiver_init(void);

/**
 * Register a callback for decoded IR commands.
 * @param cb  Callback function, or NULL to disable
 */
void ir_receiver_set_callback(ir_callback_t cb);

/**
 * Check if an IR command has been received.
 * @return true if a command is pending
 */
bool ir_receiver_has_command(void);

/**
 * Get the last received IR command.
 * Clears the pending flag.
 * @return Decoded command value
 */
uint32_t ir_receiver_get_command(void);

#endif /* IR_RECEIVER_H */
