#ifndef BOARD_COMMON_H
#define BOARD_COMMON_H

/**
 * Common board interface.
 *
 * Every board must implement these functions.
 */

/**
 * Initialize board-specific hardware (clocks, pins, peripherals).
 * Called once from main() before app_init().
 */
void board_init(void);

#endif /* BOARD_COMMON_H */
