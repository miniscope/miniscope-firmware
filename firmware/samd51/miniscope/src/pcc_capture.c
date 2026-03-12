#include "fw_config.h"

#ifdef FW_ENABLE_DMA

#include "pcc_capture.h"
#include "board.h"
#include "hal_gpio.h"
#include "hal_clock.h"
#include "hal_dmac.h"
#include "miniscope_config.h"
#include "sam.h"
#include <stddef.h>

/*
 * PCC capture with DMA linked-list descriptors.
 *
 * PCC mode: 8-bit input (ISIZE=0), 4-byte packing (DSIZE=2)
 *   -> PCC_RHR holds 4 pixels per read
 *   -> DMA beat size = 32-bit word
 *   -> beat_count = buffer_size / 4
 *
 * DMA channel 0 uses linked-list descriptors in a circular chain.
 * Each descriptor transfers one buffer's worth of data.
 * Transfer-complete interrupt advances the buffer pool write index.
 */

/* Capture buffer RAM — statically allocated */
static uint8_t s_buffers[MINISCOPE_NUM_BUFFERS][MINISCOPE_BUFFER_SIZE]
    __attribute__((aligned(4)));

/* DMA descriptors (must be 16-byte aligned) */
DMAC_DESCRIPTOR_ARRAY(s_base_desc);
DMAC_DESCRIPTOR_ARRAY(s_wb_desc);

/* Extra descriptors for the linked list (one per buffer) */
static dmac_descriptor_registers_t s_ll_desc[MINISCOPE_NUM_BUFFERS]
    __attribute__((aligned(16)));

static buffer_pool_t *s_pool;
static bool s_active;

/* DMA channel used for PCC capture */
#define PCC_DMA_CHANNEL 0

static void pcc_pinmux_init(void)
{
    /* Data lines: PA16-PA23 -> PMUX K */
    for (uint8_t pin = 16; pin <= 23; pin++) {
        gpio_set_pmux(0, pin, PORT_PMUX_PMUXE_K_Val);
    }
    /* DEN1: PA12, DEN2: PA13, CLK: PA14 -> PMUX K */
    gpio_set_pmux(0, 12, PORT_PMUX_PMUXE_K_Val);
    gpio_set_pmux(0, 13, PORT_PMUX_PMUXO_K_Val);
    gpio_set_pmux(0, 14, PORT_PMUX_PMUXE_K_Val);
}

static void pcc_peripheral_init(void)
{
    /* Enable MCLK for PCC (APB D bus) */
    hal_clock_enable_apb(&MCLK_REGS->MCLK_APBDMASK, MCLK_APBDMASK_PCC_Msk);

    /* PCC mode register:
     *   ISIZE=0: 8-bit input data
     *   DSIZE=2: 4 bytes packing (4 pixels per PCC_RHR read)
     *   PCEN=1:  Parallel capture enable
     */
    PCC_REGS->PCC_MR = PCC_MR_ISIZE(0)
                      | PCC_MR_DSIZE(2)
                      | PCC_MR_PCEN_Msk;
}

static void pcc_dma_init(void)
{
    /* Enable MCLK for DMAC */
    MCLK_REGS->MCLK_AHBMASK |= MCLK_AHBMASK_DMAC_Msk;

    /* Initialize DMAC with descriptor tables */
    dmac_init(s_base_desc, s_wb_desc);

    /* Build linked-list descriptors — one per buffer, circular */
    uint16_t beat_count = MINISCOPE_BUFFER_SIZE / 4;

    for (uint8_t i = 0; i < MINISCOPE_NUM_BUFFERS; i++) {
        dmac_descriptor_config_t cfg = {
            .src_addr   = (const void *)&PCC_REGS->PCC_RHR,
            .dst_addr   = s_buffers[i],
            .beat_count = beat_count,
            .btctrl     = DMAC_BTCTRL_VALID_Msk
                        | DMAC_BTCTRL_BEATSIZE_WORD
                        | DMAC_BTCTRL_DSTINC_Msk
                        | DMAC_BTCTRL_BLOCKACT_INT,  /* Interrupt on block complete */
            .next       = NULL, /* Will be set by dmac_link_descriptors */
        };
        dmac_descriptor_fill(&s_ll_desc[i], &cfg);
    }

    /* Link descriptors in circular chain */
    dmac_link_descriptors(s_ll_desc, MINISCOPE_NUM_BUFFERS, true);

    /* Copy first descriptor to base descriptor (channel 0) */
    s_base_desc[PCC_DMA_CHANNEL] = s_ll_desc[0];

    /* Configure DMA channel 0 */
    DMAC_REGS->CHANNEL[PCC_DMA_CHANNEL].DMAC_CHCTRLA =
        DMAC_CHCTRLA_TRIGSRC(PCC_DMAC_ID_RX)
        | DMAC_CHCTRLA_TRIGACT_BURST
        | DMAC_CHCTRLA_BURSTLEN_SINGLE;

    /* Enable transfer complete interrupt */
    DMAC_REGS->CHANNEL[PCC_DMA_CHANNEL].DMAC_CHINTENSET =
        DMAC_CHINTENSET_TCMPL_Msk;

    /* Enable DMAC channel 0 IRQ in NVIC */
    NVIC_EnableIRQ(DMAC_0_IRQn);
}

void pcc_capture_init(buffer_pool_t *pool)
{
    s_pool = pool;
    s_active = false;

    pcc_pinmux_init();
    pcc_peripheral_init();
    pcc_dma_init();
}

void pcc_capture_start(void)
{
    if (s_active) return;

    buffer_pool_reset(s_pool);
    s_active = true;

    /* Enable PCC */
    PCC_REGS->PCC_MR |= PCC_MR_PCEN_Msk;

    /* Enable DMA channel */
    DMAC_REGS->CHANNEL[PCC_DMA_CHANNEL].DMAC_CHCTRLA |=
        DMAC_CHCTRLA_ENABLE_Msk;
}

void pcc_capture_stop(void)
{
    if (!s_active) return;

    /* Disable DMA channel */
    DMAC_REGS->CHANNEL[PCC_DMA_CHANNEL].DMAC_CHCTRLA &=
        ~DMAC_CHCTRLA_ENABLE_Msk;

    s_active = false;
}

bool pcc_capture_is_active(void)
{
    return s_active;
}

/* DMA channel 0 interrupt handler */
void DMAC_0_Handler(void)
{
    if (DMAC_REGS->CHANNEL[PCC_DMA_CHANNEL].DMAC_CHINTFLAG
        & DMAC_CHINTFLAG_TCMPL_Msk)
    {
        /* Clear interrupt flag */
        DMAC_REGS->CHANNEL[PCC_DMA_CHANNEL].DMAC_CHINTFLAG =
            DMAC_CHINTFLAG_TCMPL_Msk;

        /* Advance buffer pool write index */
        if (s_pool) {
            buffer_pool_mark_filled(s_pool);
        }
    }
}

#endif /* FW_ENABLE_DMA */
