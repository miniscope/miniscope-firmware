#include "fw_config.h"

#ifdef FW_ENABLE_BATTERY_MONITOR

#include "battery_monitor.h"
#include "board.h"
#include "hal_adc.h"
#include "hal_clock.h"
#include "miniscope_config.h"

void battery_monitor_init(void)
{
    /* Enable MCLK for ADC0 (APB D bus) */
    hal_clock_enable_apb(&MCLK_REGS->MCLK_APBDMASK, MCLK_APBDMASK_ADC0_Msk);

    /* Route GCLK0 to ADC0 */
    hal_clock_enable_gclk_channel(ADC0_GCLK_ID, 0);

    /* Initialize ADC0: VDDANA/2 reference, 12-bit, DIV256 prescaler */
    hal_adc_config_t cfg = {
        .ref        = HAL_ADC_REF_INTVCC0,  /* VDDANA / 2 */
        .resolution = HAL_ADC_RES_12BIT,
        .prescaler  = 7,                     /* DIV256 for slow, stable readings */
    };
    hal_adc_init(ADC0_REGS, &cfg);
}

uint16_t battery_read_adc(void)
{
    return hal_adc_read(ADC0_REGS, BOARD_ADC_BATT_AIN);
}

bool battery_is_low(void)
{
    uint16_t val = battery_read_adc();
    return val < MINISCOPE_BATT_LOW_THRESHOLD;
}

#endif /* FW_ENABLE_BATTERY_MONITOR */
