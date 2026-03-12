#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <stdint.h>

/**
 * Excitation LED PWM control via TC0 on PB30.
 *
 * The LED is controlled by:
 *   - PWM output on PB30 (TC0/WO[0])
 *   - Enable line on PB01 (active high)
 *
 * Guarded by FW_ENABLE_LED_PWM.
 */

/** Initialize TC0 PWM for LED control. */
void led_control_init(void);

/**
 * Set LED duty cycle.
 * @param duty  Duty cycle 0-100 (0 = off, 100 = full brightness)
 */
void led_control_set_duty(uint8_t duty);

/** Enable the LED (assert enable line + start PWM). */
void led_control_enable(void);

/** Disable the LED (deassert enable line + stop PWM). */
void led_control_disable(void);

#endif /* LED_CONTROL_H */
