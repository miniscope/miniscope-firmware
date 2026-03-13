#include "hal_adc.h"

#define ADC_SYNC_TIMEOUT 10000U

static void adc_sync_wait(adc_registers_t *adc, uint32_t mask)
{
    uint32_t timeout = ADC_SYNC_TIMEOUT;
    while ((adc->ADC_SYNCBUSY & mask) && --timeout) {
        /* wait */
    }
    if (!timeout) {
        __BKPT(0);
    }
}

void hal_adc_init(adc_registers_t *adc, const hal_adc_config_t *config)
{
    /* Software reset */
    adc->ADC_CTRLA = ADC_CTRLA_SWRST_Msk;
    adc_sync_wait(adc, ADC_SYNCBUSY_SWRST_Msk);

    /* Prescaler */
    adc->ADC_CTRLA = ADC_CTRLA_PRESCALER(config->prescaler & 0x7);

    /* Reference */
    adc->ADC_REFCTRL = ADC_REFCTRL_REFSEL(config->ref);

    /* Resolution */
    adc->ADC_CTRLB = ADC_CTRLB_RESSEL(config->resolution);
    adc_sync_wait(adc, ADC_SYNCBUSY_CTRLB_Msk);

    /* Single-ended mode (ground on negative input) */
    adc->ADC_INPUTCTRL = ADC_INPUTCTRL_MUXNEG_GND;
    adc_sync_wait(adc, ADC_SYNCBUSY_INPUTCTRL_Msk);

    /* Enable */
    adc->ADC_CTRLA |= ADC_CTRLA_ENABLE_Msk;
    adc_sync_wait(adc, ADC_SYNCBUSY_ENABLE_Msk);
}

uint16_t hal_adc_read(adc_registers_t *adc, uint8_t channel)
{
    /* Select input channel */
    adc->ADC_INPUTCTRL = (adc->ADC_INPUTCTRL & ~ADC_INPUTCTRL_MUXPOS_Msk)
                       | ADC_INPUTCTRL_MUXPOS(channel);
    adc_sync_wait(adc, ADC_SYNCBUSY_INPUTCTRL_Msk);

    /* Start conversion */
    adc->ADC_SWTRIG = ADC_SWTRIG_START_Msk;
    adc_sync_wait(adc, ADC_SYNCBUSY_SWTRIG_Msk);

    /* Wait for result */
    uint32_t timeout = ADC_SYNC_TIMEOUT;
    while (!(adc->ADC_INTFLAG & ADC_INTFLAG_RESRDY_Msk) && --timeout) {
        /* wait */
    }
    if (!timeout) {
        __BKPT(0);
    }

    /* Clear flag and return result */
    adc->ADC_INTFLAG = ADC_INTFLAG_RESRDY_Msk;
    return (uint16_t)adc->ADC_RESULT;
}
