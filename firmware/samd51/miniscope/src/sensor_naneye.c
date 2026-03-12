#include "fw_config.h"

#ifdef FW_SENSOR_NANEYE

#include "sensor_naneye.h"
#include "board.h"
#include "hal_gpio.h"
#include "hal_clock.h"

/*
 * NanEye sensor driver stub.
 *
 * The NanEye is a tiny endoscopic image sensor that communicates
 * over SERCOM SPI. Full implementation depends on the specific
 * NanEye variant and SERCOM assignment from the board config.
 *
 * This stub provides the interface so boards with FW_SENSOR_NANEYE
 * can compile. The actual SPI protocol will be implemented when
 * hardware is available for testing.
 */

void sensor_naneye_init(void)
{
    /* TODO: Configure SERCOM SPI for NanEye communication */
}

void sensor_naneye_start(void)
{
    /* TODO: Start continuous frame output */
}

void sensor_naneye_stop(void)
{
    /* TODO: Stop frame output */
}

#endif /* FW_SENSOR_NANEYE */
