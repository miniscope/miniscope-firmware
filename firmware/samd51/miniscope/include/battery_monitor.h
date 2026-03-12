#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Battery voltage monitor via ADC0 on PA02 (AIN0).
 *
 * Guarded by FW_ENABLE_BATTERY_MONITOR.
 */

/** Initialize ADC0 for battery monitoring. */
void battery_monitor_init(void);

/** Read the current battery ADC value. */
uint16_t battery_read_adc(void);

/**
 * Check if battery is below the low threshold.
 * @return true if battery voltage is critically low
 */
bool battery_is_low(void);

#endif /* BATTERY_MONITOR_H */
