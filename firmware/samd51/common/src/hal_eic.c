#include "hal_eic.h"

#define EIC_LINE_COUNT 16
#define EIC_SYNC_TIMEOUT 10000U

static hal_eic_callback_t s_callbacks[EIC_LINE_COUNT];

static void eic_sync_wait(uint32_t mask)
{
    uint32_t timeout = EIC_SYNC_TIMEOUT;
    while ((EIC_REGS->EIC_SYNCBUSY & mask) && --timeout) {
        /* wait */
    }
    if (!timeout) {
        __BKPT(0);
    }
}

void hal_eic_init(void)
{
    /* Software reset */
    EIC_REGS->EIC_CTRLA = EIC_CTRLA_SWRST_Msk;
    eic_sync_wait(EIC_SYNCBUSY_SWRST_Msk);

    /* Enable EIC */
    EIC_REGS->EIC_CTRLA = EIC_CTRLA_ENABLE_Msk;
    eic_sync_wait(EIC_SYNCBUSY_ENABLE_Msk);
}

void hal_eic_configure_line(const hal_eic_config_t *config)
{
    if (config->extint >= EIC_LINE_COUNT) {
        __BKPT(0);
        return;
    }

    /* Must disable EIC to change CONFIG registers */
    EIC_REGS->EIC_CTRLA &= ~EIC_CTRLA_ENABLE_Msk;
    eic_sync_wait(EIC_SYNCBUSY_ENABLE_Msk);

    /*
     * EIC_CONFIG registers: two registers (CONFIG[0] for EXTINT 0-7,
     * CONFIG[1] for EXTINT 8-15). Each EXTINT uses 4 bits:
     *   SENSE[2:0] + FILTEN[3]
     */
    uint8_t reg_idx = config->extint / 8;
    uint8_t pos = (config->extint % 8) * 4;
    uint32_t mask = 0xFu << pos;
    uint32_t val = ((uint32_t)config->sense & 0x7u);
    if (config->filter) {
        val |= 0x8u;  /* FILTEN bit */
    }

    EIC_REGS->EIC_CONFIG[reg_idx] =
        (EIC_REGS->EIC_CONFIG[reg_idx] & ~mask) | (val << pos);

    /* Debounce */
    if (config->debounce) {
        EIC_REGS->EIC_DEBOUNCEN |= (1u << config->extint);
    } else {
        EIC_REGS->EIC_DEBOUNCEN &= ~(1u << config->extint);
    }

    /* Re-enable EIC */
    EIC_REGS->EIC_CTRLA |= EIC_CTRLA_ENABLE_Msk;
    eic_sync_wait(EIC_SYNCBUSY_ENABLE_Msk);
}

void hal_eic_enable_line(uint8_t extint)
{
    if (extint >= EIC_LINE_COUNT) return;
    EIC_REGS->EIC_INTENSET = (1u << extint);
}

void hal_eic_disable_line(uint8_t extint)
{
    if (extint >= EIC_LINE_COUNT) return;
    EIC_REGS->EIC_INTENCLR = (1u << extint);
}

void hal_eic_set_callback(uint8_t extint, hal_eic_callback_t cb)
{
    if (extint >= EIC_LINE_COUNT) return;
    s_callbacks[extint] = cb;
}

void hal_eic_irq_handler(void)
{
    uint32_t flags = EIC_REGS->EIC_INTFLAG & EIC_REGS->EIC_INTENSET;

    while (flags) {
        uint8_t line = (uint8_t)__builtin_ctz(flags);
        EIC_REGS->EIC_INTFLAG = (1u << line); /* W1C */

        if (s_callbacks[line]) {
            s_callbacks[line](line);
        }

        flags &= ~(1u << line);
    }
}
