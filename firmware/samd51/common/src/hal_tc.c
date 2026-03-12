#include "hal_tc.h"

/*
 * SAMD51 TC instance count varies by package:
 *   100-pin (P20A): TC0-TC7 (8 instances)
 *   64-pin  (J20A): TC0-TC5 (6 instances)
 * Use HAL_TC_INST_NUM from the DFP device header.
 */

/* Map TC base address to index for callback storage */
static tc_registers_t *const s_tc_bases[HAL_TC_INST_NUM] = {
    TC0_REGS, TC1_REGS, TC2_REGS, TC3_REGS,
    TC4_REGS, TC5_REGS,
#if HAL_TC_INST_NUM > 6
    TC6_REGS, TC7_REGS,
#endif
};

static hal_tc_callback_t s_ovf_callbacks[HAL_TC_INST_NUM];

static int tc_index(tc_registers_t *tc)
{
    for (int i = 0; i < HAL_TC_INST_NUM; i++) {
        if (s_tc_bases[i] == tc) return i;
    }
    __BKPT(0);
    return -1;
}

static void tc_sync_wait(tc_registers_t *tc, uint32_t mask)
{
    uint32_t timeout = 10000U;
    while ((tc->COUNT16.TC_SYNCBUSY & mask) && --timeout) {
        /* wait */
    }
    if (!timeout) {
        __BKPT(0);
    }
}

void hal_tc_init(tc_registers_t *tc, hal_tc_mode_t mode, uint8_t prescaler)
{
    /* Software reset */
    tc->COUNT16.TC_CTRLA = TC_CTRLA_SWRST_Msk;
    tc_sync_wait(tc, TC_SYNCBUSY_SWRST_Msk);

    uint32_t ctrla = TC_CTRLA_PRESCALER(prescaler & 0x7);

    switch (mode) {
    case HAL_TC_MODE_COUNT16:
        ctrla |= TC_CTRLA_MODE_COUNT16;
        break;
    case HAL_TC_MODE_NPWM:
        ctrla |= TC_CTRLA_MODE_COUNT16;
        /* NPWM uses CC0 as period, CC1 as duty — set wavegen in WAVE register */
        break;
    }

    tc->COUNT16.TC_CTRLA = ctrla;
    tc_sync_wait(tc, TC_SYNCBUSY_ENABLE_Msk);

    if (mode == HAL_TC_MODE_NPWM) {
        tc->COUNT16.TC_WAVE = TC_WAVE_WAVEGEN_NPWM;
    }
}

void hal_tc_set_period(tc_registers_t *tc, uint16_t period)
{
    tc->COUNT16.TC_CC[0] = period;
    tc_sync_wait(tc, TC_SYNCBUSY_CC0_Msk);
}

void hal_tc_set_compare(tc_registers_t *tc, uint16_t compare)
{
    tc->COUNT16.TC_CC[1] = compare;
    tc_sync_wait(tc, TC_SYNCBUSY_CC1_Msk);
}

void hal_tc_set_duty_percent(tc_registers_t *tc, uint8_t duty)
{
    if (duty > 100) duty = 100;
    uint16_t period = tc->COUNT16.TC_CC[0];
    uint16_t compare = (uint16_t)((uint32_t)period * duty / 100U);
    hal_tc_set_compare(tc, compare);
}

void hal_tc_start(tc_registers_t *tc)
{
    tc->COUNT16.TC_CTRLA |= TC_CTRLA_ENABLE_Msk;
    tc_sync_wait(tc, TC_SYNCBUSY_ENABLE_Msk);
}

void hal_tc_stop(tc_registers_t *tc)
{
    tc->COUNT16.TC_CTRLA &= ~TC_CTRLA_ENABLE_Msk;
    tc_sync_wait(tc, TC_SYNCBUSY_ENABLE_Msk);
}

void hal_tc_set_overflow_callback(tc_registers_t *tc, hal_tc_callback_t cb)
{
    int idx = tc_index(tc);
    if (idx < 0) return;

    s_ovf_callbacks[idx] = cb;

    if (cb) {
        tc->COUNT16.TC_INTENSET = TC_INTENSET_OVF_Msk;
    } else {
        tc->COUNT16.TC_INTENCLR = TC_INTENCLR_OVF_Msk;
    }
}

void hal_tc_irq_handler(tc_registers_t *tc)
{
    if (tc->COUNT16.TC_INTFLAG & TC_INTFLAG_OVF_Msk) {
        tc->COUNT16.TC_INTFLAG = TC_INTFLAG_OVF_Msk; /* W1C */

        int idx = tc_index(tc);
        if (idx >= 0 && s_ovf_callbacks[idx]) {
            s_ovf_callbacks[idx]();
        }
    }
}
