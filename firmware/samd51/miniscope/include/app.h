#ifndef APP_H
#define APP_H

#include <stdint.h>

/**
 * Application initialization (called once after board_init).
 */
void app_init(void);

/**
 * Application main loop body (called repeatedly).
 */
void app_run(void);

#endif /* APP_H */
