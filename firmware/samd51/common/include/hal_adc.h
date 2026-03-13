#ifndef HAL_ADC_H
#define HAL_ADC_H

#include "sam.h"
#include <stdint.h>

/**
 * SAMD51 ADC HAL — single-shot conversion.
 *
 * The SAMD51 has two ADC instances (ADC0, ADC1). Each must be clock-gated
 * and have a GCLK source routed before use.
 *
 * GCLK channel IDs: ADC0_GCLK_ID (40), ADC1_GCLK_ID (41)
 */

/** ADC voltage reference selection */
typedef enum {
    HAL_ADC_REF_INTREF  = 0,   /* Internal bandgap reference */
    HAL_ADC_REF_INTVCC0 = 2,   /* VDDANA / 2 */
    HAL_ADC_REF_INTVCC1 = 3,   /* VDDANA */
    HAL_ADC_REF_VREFA   = 4,   /* External reference A */
    HAL_ADC_REF_VREFB   = 5,   /* External reference B */
} hal_adc_ref_t;

/** ADC resolution */
typedef enum {
    HAL_ADC_RES_12BIT = 0,
    HAL_ADC_RES_16BIT = 1,  /* Averaging mode */
    HAL_ADC_RES_10BIT = 2,
    HAL_ADC_RES_8BIT  = 3,
} hal_adc_res_t;

/** ADC configuration */
typedef struct {
    hal_adc_ref_t ref;         /* Voltage reference */
    hal_adc_res_t resolution;  /* Conversion resolution */
    uint8_t       prescaler;   /* Clock prescaler (0=DIV2 ... 7=DIV256) */
} hal_adc_config_t;

/**
 * Initialize an ADC instance.
 * The ADC peripheral clock and GCLK must be enabled by the caller.
 *
 * @param adc     ADC registers (ADC0_REGS or ADC1_REGS)
 * @param config  ADC configuration
 */
void hal_adc_init(adc_registers_t *adc, const hal_adc_config_t *config);

/**
 * Read a single ADC channel (blocking).
 *
 * @param adc      ADC registers
 * @param channel  Positive input channel (AINx, 0-23)
 * @return         Conversion result (width depends on resolution)
 */
uint16_t hal_adc_read(adc_registers_t *adc, uint8_t channel);

#endif /* HAL_ADC_H */
