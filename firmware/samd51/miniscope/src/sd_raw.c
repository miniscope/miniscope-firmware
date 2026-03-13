#include "fw_config.h"

#ifdef FW_OUTPUT_SD

#include "sd_raw.h"
#include "board.h"
#include "hal_gpio.h"
#include "hal_clock.h"
#include "hal_delay.h"
#include "sam.h"

/*
 * SDHC0 register-level SD card driver for SAMD51.
 *
 * Pin mux assignments (Wire-Free V4):
 *   PA08 = CMD   (PMUX I)
 *   PA09 = DAT0  (PMUX I)
 *   PA10 = DAT1  (PMUX I)
 *   PA11 = DAT2  (PMUX I)
 *   PB10 = DAT3  (PMUX I)
 *   PB11 = SDCK  (PMUX I)
 */

#define SD_BLOCK_SIZE 512
#define SD_CMD_TIMEOUT 100000U

/* SD commands */
#define CMD0   0   /* GO_IDLE_STATE */
#define CMD2   2   /* ALL_SEND_CID */
#define CMD3   3   /* SEND_RELATIVE_ADDR */
#define CMD6   6   /* SWITCH_FUNC */
#define CMD7   7   /* SELECT_CARD */
#define CMD8   8   /* SEND_IF_COND */
#define CMD12  12  /* STOP_TRANSMISSION */
#define CMD16  16  /* SET_BLOCKLEN */
#define CMD17  17  /* READ_SINGLE_BLOCK */
#define CMD24  24  /* WRITE_BLOCK */
#define CMD25  25  /* WRITE_MULTIPLE_BLOCK */
#define CMD55  55  /* APP_CMD */
#define ACMD6  6   /* SET_BUS_WIDTH */
#define ACMD41 41  /* SD_SEND_OP_COND */

static uint16_t s_rca; /* Relative Card Address */

static void sd_pinmux_init(void)
{
    /* CMD: PA08 PMUX I */
    gpio_set_pmux(0, 8, PORT_PMUX_PMUXE_I_Val);
    /* DAT0: PA09 PMUX I */
    gpio_set_pmux(0, 9, PORT_PMUX_PMUXO_I_Val);
    /* DAT1: PA10 PMUX I */
    gpio_set_pmux(0, 10, PORT_PMUX_PMUXE_I_Val);
    /* DAT2: PA11 PMUX I */
    gpio_set_pmux(0, 11, PORT_PMUX_PMUXO_I_Val);
    /* DAT3: PB10 PMUX I */
    gpio_set_pmux(1, 10, PORT_PMUX_PMUXE_I_Val);
    /* CLK: PB11 PMUX I */
    gpio_set_pmux(1, 11, PORT_PMUX_PMUXO_I_Val);
}

static void sd_clock_init(void)
{
    /* Enable MCLK for SDHC0 (AHB bus) */
    MCLK_REGS->MCLK_AHBMASK |= MCLK_AHBMASK_SDHC0_Msk;

    /* Route GCLK0 to SDHC0 main and slow clocks.
     * The slow clock drives the data timeout counter (SDHC_TCR). */
    hal_clock_enable_gclk_channel(SDHC0_GCLK_ID, 0);
    hal_clock_enable_gclk_channel(SDHC0_GCLK_ID_SLOW, 0);
}

static void sd_set_clock(uint16_t div)
{
    /* SD Host Controller Spec 3.2.1: clock change procedure */

    /* Disable SD clock output */
    SDHC0_REGS->SDHC_CCR &= ~SDHC_CCR_SDCLKEN_Msk;

    /* Wait for command and data lines to be free */
    uint32_t timeout = SD_CMD_TIMEOUT;
    while ((SDHC0_REGS->SDHC_PSR & (SDHC_PSR_CMDINHC_Msk | SDHC_PSR_CMDINHD_Msk)) && --timeout) {
        /* wait */
    }

    /* Write new divider: SDCLKFSEL (bits 15:8) + USDCLKFSEL (bits 7:6) */
    uint16_t ccr = SDHC_CCR_INTCLKEN_Msk
                 | SDHC_CCR_SDCLKFSEL(div & 0xFF)
                 | SDHC_CCR_USDCLKFSEL((div >> 8) & 0x3);
    SDHC0_REGS->SDHC_CCR = ccr;

    /* Wait for internal clock stable */
    timeout = SD_CMD_TIMEOUT;
    while (!(SDHC0_REGS->SDHC_CCR & SDHC_CCR_INTCLKS_Msk) && --timeout) {
        /* wait */
    }

    /* Re-enable SD clock output */
    SDHC0_REGS->SDHC_CCR |= SDHC_CCR_SDCLKEN_Msk;
}

static void sd_reset_lines(uint8_t mask)
{
    SDHC0_REGS->SDHC_SRR = mask;
    uint32_t timeout = SD_CMD_TIMEOUT;
    while ((SDHC0_REGS->SDHC_SRR & mask) && --timeout) {
        /* wait for reset complete */
    }
}

static bool sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t resp_type)
{
    /* Wait for command inhibit to clear */
    uint32_t timeout = SD_CMD_TIMEOUT;
    while ((SDHC0_REGS->SDHC_PSR & SDHC_PSR_CMDINHC_Msk) && --timeout) {
        /* wait */
    }
    if (!timeout) return false;

    /* Clear all interrupt status */
    SDHC0_REGS->SDHC_NISTR = 0xFFFF;
    SDHC0_REGS->SDHC_EISTR = 0xFFFF;

    /* Set argument */
    SDHC0_REGS->SDHC_ARG1R = arg;

    /* Build command register value */
    uint16_t cmd_val = SDHC_CR_CMDIDX(cmd);
    switch (resp_type) {
    case 0: /* No response */
        break;
    case 1: /* R1/R6/R7 — 48-bit */
        cmd_val |= SDHC_CR_RESPTYP_48_BIT | SDHC_CR_CMDCCEN_Msk | SDHC_CR_CMDICEN_Msk;
        break;
    case 2: /* R2 — 136-bit */
        cmd_val |= SDHC_CR_RESPTYP_136_BIT | SDHC_CR_CMDCCEN_Msk;
        break;
    case 3: /* R3 — 48-bit, no CRC */
        cmd_val |= SDHC_CR_RESPTYP_48_BIT;
        break;
    }
    SDHC0_REGS->SDHC_CR = cmd_val;

    /* Wait for command complete */
    timeout = SD_CMD_TIMEOUT;
    while (!(SDHC0_REGS->SDHC_NISTR & SDHC_NISTR_CMDC_Msk) && --timeout) {
        if (SDHC0_REGS->SDHC_EISTR) {
            sd_reset_lines(SDHC_SRR_SWRSTCMD_Msk);
            return false;
        }
    }
    if (!timeout) {
        sd_reset_lines(SDHC_SRR_SWRSTCMD_Msk);
        return false;
    }

    SDHC0_REGS->SDHC_NISTR = SDHC_NISTR_CMDC_Msk;
    return true;
}

static bool sd_send_acmd(uint8_t cmd, uint32_t arg, uint8_t resp_type)
{
    if (!sd_send_cmd(CMD55, (uint32_t)s_rca << 16, 1)) return false;
    return sd_send_cmd(cmd, arg, resp_type);
}

sd_error_t sd_raw_init(void)
{
    sd_pinmux_init();
    sd_clock_init();

    /* Reset SDHC */
    SDHC0_REGS->SDHC_SRR = SDHC_SRR_SWRSTALL_Msk;
    uint32_t timeout = SD_CMD_TIMEOUT;
    while ((SDHC0_REGS->SDHC_SRR & SDHC_SRR_SWRSTALL_Msk) && --timeout) {
        /* wait */
    }
    if (!timeout) return SD_ERR_TIMEOUT;

    /* Set data timeout: ~2M SDCLK cycles (matches reference) */
    SDHC0_REGS->SDHC_TCR = SDHC_TCR_DTCVAL(0xE);

    /* Enable internal clock */
    SDHC0_REGS->SDHC_CCR = SDHC_CCR_INTCLKEN_Msk | SDHC_CCR_SDCLKFSEL(0x80);
    timeout = SD_CMD_TIMEOUT;
    while (!(SDHC0_REGS->SDHC_CCR & SDHC_CCR_INTCLKS_Msk) && --timeout) {
        /* wait */
    }
    if (!timeout) return SD_ERR_TIMEOUT;

    /* Enable SD clock */
    SDHC0_REGS->SDHC_CCR |= SDHC_CCR_SDCLKEN_Msk;

    /* Set bus power (3.3V) */
    SDHC0_REGS->SDHC_PCR = SDHC_PCR_SDBPWR_Msk | SDHC_PCR_SDBVSEL_3V3;

    /* Enable all normal interrupt status */
    SDHC0_REGS->SDHC_NISTER = 0xFFFF;
    SDHC0_REGS->SDHC_EISTER = 0xFFFF;

    /* CMD0: GO_IDLE_STATE */
    if (!sd_send_cmd(CMD0, 0, 0)) return SD_ERR_CMD_FAIL;

    /* CMD8: SEND_IF_COND (voltage check: 0x1AA = 3.3V + check pattern) */
    if (!sd_send_cmd(CMD8, 0x000001AA, 1)) return SD_ERR_CMD_FAIL;
    uint32_t r7 = SDHC0_REGS->SDHC_RR[0];
    if ((r7 & 0xFFF) != 0x1AA) return SD_ERR_UNSUPPORTED;

    /* ACMD41: SD_SEND_OP_COND (HCS=1 for SDHC/SDXC) */
    s_rca = 0;
    for (int i = 0; i < 1000; i++) {
        if (!sd_send_acmd(ACMD41, 0x40300000, 3)) return SD_ERR_CMD_FAIL;
        uint32_t ocr = SDHC0_REGS->SDHC_RR[0];
        if (ocr & (1u << 31)) { /* Card power-up complete */
            break;
        }
        if (i == 999) return SD_ERR_TIMEOUT;
        /* Brief delay between retries */
        delay_ms_approx(1);
    }

    /* CMD2: ALL_SEND_CID */
    if (!sd_send_cmd(CMD2, 0, 2)) return SD_ERR_CMD_FAIL;

    /* CMD3: SEND_RELATIVE_ADDR */
    if (!sd_send_cmd(CMD3, 0, 1)) return SD_ERR_CMD_FAIL;
    s_rca = (uint16_t)(SDHC0_REGS->SDHC_RR[0] >> 16);

    /* CMD7: SELECT_CARD */
    if (!sd_send_cmd(CMD7, (uint32_t)s_rca << 16, 1)) return SD_ERR_CMD_FAIL;

    /* ACMD6: SET_BUS_WIDTH to 4-bit */
    if (!sd_send_acmd(ACMD6, 2, 1)) return SD_ERR_CMD_FAIL;

    /* Set host controller to 4-bit mode */
    SDHC0_REGS->SDHC_HC1R |= SDHC_HC1R_DW_4BIT;

    /* Ramp clock: 234 kHz init → 30 MHz operational (60 MHz / (2*1)) */
    sd_set_clock(1);

    /* CMD16: SET_BLOCKLEN to 512 bytes */
    sd_send_cmd(CMD16, SD_BLOCK_SIZE, 1);

    return SD_OK;
}

sd_error_t sd_raw_write_block(uint32_t block_addr, const uint8_t *data)
{
    /* Wait for data inhibit to clear */
    uint32_t timeout = SD_CMD_TIMEOUT;
    while ((SDHC0_REGS->SDHC_PSR & SDHC_PSR_CMDINHD_Msk) && --timeout) {
        /* wait */
    }
    if (!timeout) return SD_ERR_TIMEOUT;

    /* Set block size and count */
    SDHC0_REGS->SDHC_BSR = SDHC_BSR_BLOCKSIZE(SD_BLOCK_SIZE);
    SDHC0_REGS->SDHC_BCR = 1;

    /* Transfer mode: single block write */
    SDHC0_REGS->SDHC_TMR = 0; /* Single block, write */

    /* CMD24: WRITE_BLOCK */
    if (!sd_send_cmd(CMD24, block_addr, 1)) return SD_ERR_CMD_FAIL;

    /* Write data via buffer data port */
    const uint32_t *src = (const uint32_t *)data;
    for (int i = 0; i < SD_BLOCK_SIZE / 4; i++) {
        timeout = SD_CMD_TIMEOUT;
        while (!(SDHC0_REGS->SDHC_PSR & SDHC_PSR_BUFWREN_Msk) && --timeout) {
            /* wait for buffer write ready */
        }
        if (!timeout) {
            sd_reset_lines(SDHC_SRR_SWRSTDAT_Msk);
            return SD_ERR_TIMEOUT;
        }
        SDHC0_REGS->SDHC_BDPR = src[i];
    }

    /* Wait for transfer complete */
    timeout = SD_CMD_TIMEOUT;
    while (!(SDHC0_REGS->SDHC_NISTR & SDHC_NISTR_TRFC_Msk) && --timeout) {
        if (SDHC0_REGS->SDHC_EISTR) {
            sd_reset_lines(SDHC_SRR_SWRSTCMD_Msk | SDHC_SRR_SWRSTDAT_Msk);
            return SD_ERR_CMD_FAIL;
        }
    }
    if (!timeout) return SD_ERR_TIMEOUT;

    SDHC0_REGS->SDHC_NISTR = SDHC_NISTR_TRFC_Msk;
    return SD_OK;
}

sd_error_t sd_raw_write_multi(uint32_t block_addr,
                              const uint8_t *data,
                              uint32_t block_count)
{
    if (block_count == 0) return SD_OK;
    if (block_count == 1) return sd_raw_write_block(block_addr, data);

    /* Wait for data inhibit to clear */
    uint32_t timeout = SD_CMD_TIMEOUT;
    while ((SDHC0_REGS->SDHC_PSR & SDHC_PSR_CMDINHD_Msk) && --timeout) {
        /* wait */
    }
    if (!timeout) return SD_ERR_TIMEOUT;

    /* Set block size and count */
    SDHC0_REGS->SDHC_BSR = SDHC_BSR_BLOCKSIZE(SD_BLOCK_SIZE);
    SDHC0_REGS->SDHC_BCR = (uint16_t)block_count;

    /* Transfer mode: multi-block write with block count enable */
    SDHC0_REGS->SDHC_TMR = SDHC_TMR_MSBSEL_Msk | SDHC_TMR_BCEN_Msk;

    /* CMD25: WRITE_MULTIPLE_BLOCK */
    if (!sd_send_cmd(CMD25, block_addr, 1)) return SD_ERR_CMD_FAIL;

    /* Write all blocks via buffer data port */
    const uint32_t *src = (const uint32_t *)data;
    uint32_t total_words = (block_count * SD_BLOCK_SIZE) / 4;
    for (uint32_t i = 0; i < total_words; i++) {
        timeout = SD_CMD_TIMEOUT;
        while (!(SDHC0_REGS->SDHC_PSR & SDHC_PSR_BUFWREN_Msk) && --timeout) {
            /* wait for buffer write ready */
        }
        if (!timeout) {
            sd_reset_lines(SDHC_SRR_SWRSTDAT_Msk);
            return SD_ERR_TIMEOUT;
        }
        SDHC0_REGS->SDHC_BDPR = src[i];
    }

    /* Wait for transfer complete */
    timeout = SD_CMD_TIMEOUT;
    while (!(SDHC0_REGS->SDHC_NISTR & SDHC_NISTR_TRFC_Msk) && --timeout) {
        if (SDHC0_REGS->SDHC_EISTR) {
            sd_reset_lines(SDHC_SRR_SWRSTCMD_Msk | SDHC_SRR_SWRSTDAT_Msk);
            return SD_ERR_CMD_FAIL;
        }
    }
    if (!timeout) return SD_ERR_TIMEOUT;

    SDHC0_REGS->SDHC_NISTR = SDHC_NISTR_TRFC_Msk;
    return SD_OK;
}

sd_error_t sd_raw_read_block(uint32_t block_addr, uint8_t *data)
{
    /* Wait for data inhibit to clear */
    uint32_t timeout = SD_CMD_TIMEOUT;
    while ((SDHC0_REGS->SDHC_PSR & SDHC_PSR_CMDINHD_Msk) && --timeout) {
        /* wait */
    }
    if (!timeout) return SD_ERR_TIMEOUT;

    /* Set block size and count */
    SDHC0_REGS->SDHC_BSR = SDHC_BSR_BLOCKSIZE(SD_BLOCK_SIZE);
    SDHC0_REGS->SDHC_BCR = 1;

    /* Transfer mode: single block read */
    SDHC0_REGS->SDHC_TMR = SDHC_TMR_DTDSEL_Msk; /* Read */

    /* CMD17: READ_SINGLE_BLOCK */
    if (!sd_send_cmd(CMD17, block_addr, 1)) return SD_ERR_CMD_FAIL;

    /* Read data via buffer data port */
    uint32_t *dst = (uint32_t *)data;
    for (int i = 0; i < SD_BLOCK_SIZE / 4; i++) {
        timeout = SD_CMD_TIMEOUT;
        while (!(SDHC0_REGS->SDHC_PSR & SDHC_PSR_BUFRDEN_Msk) && --timeout) {
            /* wait for buffer ready */
        }
        if (!timeout) {
            sd_reset_lines(SDHC_SRR_SWRSTDAT_Msk);
            return SD_ERR_TIMEOUT;
        }
        dst[i] = SDHC0_REGS->SDHC_BDPR;
    }

    /* Wait for transfer complete */
    timeout = SD_CMD_TIMEOUT;
    while (!(SDHC0_REGS->SDHC_NISTR & SDHC_NISTR_TRFC_Msk) && --timeout) {
        if (SDHC0_REGS->SDHC_EISTR) {
            sd_reset_lines(SDHC_SRR_SWRSTCMD_Msk | SDHC_SRR_SWRSTDAT_Msk);
            return SD_ERR_CMD_FAIL;
        }
    }
    if (!timeout) return SD_ERR_TIMEOUT;

    SDHC0_REGS->SDHC_NISTR = SDHC_NISTR_TRFC_Msk;
    return SD_OK;
}

bool sd_raw_card_present(void)
{
    return (SDHC0_REGS->SDHC_PSR & SDHC_PSR_CARDINS_Msk) != 0;
}

#endif /* FW_OUTPUT_SD */
