#ifndef MINISCOPE_CONFIG_H
#define MINISCOPE_CONFIG_H

/**
 * Miniscope firmware configuration defaults.
 *
 * These can be overridden by board_config.cmake via BOARD_DEFINITIONS.
 */

/* Number of DMA capture buffers */
#ifndef MINISCOPE_NUM_BUFFERS
#define MINISCOPE_NUM_BUFFERS       8
#endif

/* Size of each capture buffer in bytes (20KB) */
#ifndef MINISCOPE_BUFFER_SIZE
#define MINISCOPE_BUFFER_SIZE       (20 * 1024)
#endif

/* Words per metadata header at the start of each buffer */
#ifndef MINISCOPE_META_WORDS
#define MINISCOPE_META_WORDS        12
#endif

/* Battery voltage threshold (ADC counts) below which recording stops.
 * 12-bit ADC with VDDANA/2 reference, 1/5 voltage divider:
 *   ADC = (V_batt / 5) / (VDDANA / 2) * 4095
 *   For 3.4V cutoff: (3.4/5) / 1.65 * 4095 ≈ 1688 */
#ifndef MINISCOPE_BATT_LOW_THRESHOLD
#define MINISCOPE_BATT_LOW_THRESHOLD 1688
#endif

/* Default LED PWM duty cycle (0-100) */
#ifndef MINISCOPE_LED_DEFAULT_DUTY
#define MINISCOPE_LED_DEFAULT_DUTY  0
#endif

/* Default EWL voltage (0-255) */
#ifndef MINISCOPE_EWL_DEFAULT
#define MINISCOPE_EWL_DEFAULT       128
#endif

/* SD card header block address */
#ifndef MINISCOPE_SD_HEADER_BLOCK
#define MINISCOPE_SD_HEADER_BLOCK   1022
#endif

/* SD card config block address */
#ifndef MINISCOPE_SD_CONFIG_BLOCK
#define MINISCOPE_SD_CONFIG_BLOCK   1023
#endif

/* SD card data start block */
#ifndef MINISCOPE_SD_DATA_START
#define MINISCOPE_SD_DATA_START     1024
#endif

/* Image dimensions */
#ifndef MINISCOPE_WIDTH
#define MINISCOPE_WIDTH             608
#endif

#ifndef MINISCOPE_HEIGHT
#define MINISCOPE_HEIGHT            608
#endif

#ifndef MINISCOPE_BINNING
#define MINISCOPE_BINNING           2
#endif

#endif /* MINISCOPE_CONFIG_H */
