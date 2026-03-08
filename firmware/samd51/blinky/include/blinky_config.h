#ifndef BLINKY_CONFIG_H
#define BLINKY_CONFIG_H

/**
 * Blinky project configuration.
 *
 * These defaults can be overridden by board_config.cmake via
 * target_compile_definitions. The board_config.cmake definitions
 * propagate to all linked libraries via the fw_<board>.elf target.
 */

#ifndef BLINKY_LED_PORT
#define BLINKY_LED_PORT 0  /* Port A */
#endif

#ifndef BLINKY_LED_PIN
#define BLINKY_LED_PIN 16  /* PA16 */
#endif

#ifndef BLINKY_DELAY_MS
#define BLINKY_DELAY_MS 500
#endif

#endif /* BLINKY_CONFIG_H */
