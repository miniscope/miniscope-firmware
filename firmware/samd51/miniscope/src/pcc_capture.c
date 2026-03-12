#include "fw_config.h"

#ifdef FW_ENABLE_DMA

#include "pcc_capture.h"
#include "board.h"
#include "hal_gpio.h"
#include "hal_clock.h"
#include "hal_dmac.h"
#include "hal_eic.h"
#include "hal_systick.h"
#include "miniscope_config.h"
#include "sd_format.h"
#include "sam.h"
#include <stddef.h>
#include <string.h>

/*
 * PCC capture with DMA linked-list descriptors.
 *
 * PCC mode: 8-bit input (ISIZE=0), 4-byte packing (DSIZE=2), CID=3 (DEN1+DEN2)
 *   -> PCC_RHR holds 4 pixels per read
 *   -> DMA beat size = 32-bit word
 *   -> beat_count = (buffer_size / 4) - META_WORDS (offset for header)
 *
 * DMA channel 0 uses linked-list descriptors in a circular chain.
 * Each descriptor transfers one buffer's worth of data, starting
 * 48 bytes (12 words) into the buffer to leave room for the metadata header.
 *
 * Frame boundary detection uses the frame-valid signal (EXTINT14, falling edge)
 * to handle partial buffers at frame boundaries.
 */

/* Data beat count per buffer (excluding the 12-word metadata header) */
#define PCC_BEAT_COUNT ((MINISCOPE_BUFFER_SIZE / 4) - MINISCOPE_META_WORDS)

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

/* Recording state counters (shared between ISRs and main loop) */
static volatile uint32_t s_buffer_count;
static volatile uint32_t s_frame_num;
static volatile uint32_t s_frame_buffer_count;
static volatile uint32_t s_dropped_count;
static volatile uint32_t s_record_start_ms;

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
     *   CID=3:   Both DEN1 and DEN2 required (matching reference)
     *   ISIZE=0: 8-bit input data
     *   DSIZE=2: 4 bytes packing (4 pixels per PCC_RHR read)
     *   PCEN=1:  Parallel capture enable
     */
    PCC_REGS->PCC_MR = PCC_MR_CID(0x3)
                      | PCC_MR_ISIZE(0)
                      | PCC_MR_DSIZE(2)
                      | PCC_MR_PCEN_Msk;
}

static void stamp_buffer_header(uint8_t buf_idx, uint32_t data_words)
{
    sd_buffer_meta_t meta;
    memset(&meta, 0, sizeof(meta));
    meta.header_length      = MINISCOPE_META_WORDS;
    meta.linked_list_pos    = buf_idx;
    meta.frame_number       = s_frame_num;
    meta.buffer_count       = s_buffer_count;
    meta.frame_buffer_count = s_frame_buffer_count;
    meta.write_buffer_count = 0;  /* Updated by main loop before SD write */
    meta.dropped_buffer_count = s_dropped_count;
    meta.timestamp_ms       = hal_systick_get_ticks() - s_record_start_ms;
    meta.data_length        = data_words * 4;
    meta.write_timestamp_ms = 0;  /* Updated by main loop before SD write */
    meta.battery_adc        = 0;  /* Updated by main loop */
    meta.ewl_value          = 0;  /* Updated by main loop */
    sd_format_write_meta(s_buffers[buf_idx], &meta);
}

static void pcc_dma_init(void)
{
    /* Enable MCLK for DMAC */
    MCLK_REGS->MCLK_AHBMASK |= MCLK_AHBMASK_DMAC_Msk;

    /* Initialize DMAC with descriptor tables */
    dmac_init(s_base_desc, s_wb_desc);

    /* Build linked-list descriptors — one per buffer, circular.
     * DMA destination is offset by MINISCOPE_META_WORDS * 4 bytes
     * to leave room for the metadata header at the start of each buffer. */
    for (uint8_t i = 0; i < MINISCOPE_NUM_BUFFERS; i++) {
        dmac_descriptor_config_t cfg = {
            .src_addr   = (const void *)&PCC_REGS->PCC_RHR,
            .dst_addr   = &s_buffers[i][MINISCOPE_META_WORDS * 4],
            .beat_count = PCC_BEAT_COUNT,
            .btctrl     = DMAC_BTCTRL_VALID_Msk
                        | DMAC_BTCTRL_BEATSIZE_WORD
                        | DMAC_BTCTRL_DSTINC_Msk
                        | DMAC_BTCTRL_BLOCKACT_INT,
            .next       = NULL,
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
    s_buffer_count = 0;
    s_frame_num = 0;
    s_frame_buffer_count = 0;
    s_dropped_count = 0;
    s_record_start_ms = 0;

    pcc_pinmux_init();
    pcc_peripheral_init();
    pcc_dma_init();
}

void pcc_capture_start(void)
{
    if (s_active) return;

    buffer_pool_reset(s_pool);
    s_buffer_count = 0;
    s_frame_num = 0;
    s_frame_buffer_count = 0;
    s_dropped_count = 0;
    s_record_start_ms = hal_systick_get_ticks();
    s_active = true;

    /* Reset DMA descriptor chain to start at buffer 0 */
    s_base_desc[PCC_DMA_CHANNEL] = s_ll_desc[0];

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

    /* Disable PCC */
    PCC_REGS->PCC_MR &= ~PCC_MR_PCEN_Msk;

    s_active = false;
}

bool pcc_capture_is_active(void)
{
    return s_active;
}

uint32_t pcc_capture_get_buffer_count(void)
{
    return s_buffer_count;
}

uint32_t pcc_capture_get_frame_num(void)
{
    return s_frame_num;
}

uint32_t pcc_capture_get_dropped_count(void)
{
    return s_dropped_count;
}

uint8_t *pcc_capture_get_buffer(uint8_t idx)
{
    if (idx >= MINISCOPE_NUM_BUFFERS) return NULL;
    return s_buffers[idx];
}

/*
 * Frame-valid falling-edge callback (EXTINT14).
 *
 * Called at each frame boundary. Implements the reference firmware's
 * frameValid_cb() logic:
 *   1. Disable PCC
 *   2. Disable DMA
 *   3. Read write-back BTCNT for remaining beats
 *   4. Calculate actual data words transferred
 *   5. Stamp partial buffer header
 *   6. Advance counters, mark buffer filled
 *   7. Re-enable DMA + PCC for next frame
 */
void pcc_frame_valid_callback(uint8_t extint)
{
    (void)extint;

    if (!s_active) return;

    /* 1. Disable PCC (stop capturing) */
    PCC_REGS->PCC_MR &= ~PCC_MR_PCEN_Msk;

    /* 2. Disable DMA channel */
    DMAC_REGS->CHANNEL[PCC_DMA_CHANNEL].DMAC_CHCTRLA &=
        ~DMAC_CHCTRLA_ENABLE_Msk;

    /* 3. Read remaining beats from write-back descriptor */
    uint16_t remaining = dmac_get_writeback_btcnt(s_wb_desc, PCC_DMA_CHANNEL);

    /* 4. Calculate actual data words transferred */
    uint32_t data_words = PCC_BEAT_COUNT - remaining;

    /* 5. Stamp the current buffer's metadata header with partial data */
    uint8_t buf_idx = s_buffer_count % MINISCOPE_NUM_BUFFERS;
    stamp_buffer_header(buf_idx, data_words);

    /* 6. Advance counters */
    s_buffer_count++;
    s_frame_num++;
    s_frame_buffer_count = 0;

    /* Mark buffer as filled for the main loop to write to SD */
    if (s_pool) {
        buffer_pool_mark_filled(s_pool);
    }

    /* 7. Re-enable DMA + PCC for next frame.
     * Point to the next descriptor in the linked list. */
    uint8_t next_idx = s_buffer_count % MINISCOPE_NUM_BUFFERS;
    s_base_desc[PCC_DMA_CHANNEL] = s_ll_desc[next_idx];

    DMAC_REGS->CHANNEL[PCC_DMA_CHANNEL].DMAC_CHCTRLA |=
        DMAC_CHCTRLA_ENABLE_Msk;

    PCC_REGS->PCC_MR |= PCC_MR_PCEN_Msk;
}

/* DMA channel 0 interrupt handler — full buffer complete */
void DMAC_0_Handler(void)
{
    if (DMAC_REGS->CHANNEL[PCC_DMA_CHANNEL].DMAC_CHINTFLAG
        & DMAC_CHINTFLAG_TCMPL_Msk)
    {
        /* Clear interrupt flag */
        DMAC_REGS->CHANNEL[PCC_DMA_CHANNEL].DMAC_CHINTFLAG =
            DMAC_CHINTFLAG_TCMPL_Msk;

        /* Stamp full-buffer header */
        uint8_t buf_idx = s_buffer_count % MINISCOPE_NUM_BUFFERS;
        stamp_buffer_header(buf_idx, PCC_BEAT_COUNT);

        /* Advance counters */
        s_buffer_count++;
        s_frame_buffer_count++;

        /* Advance buffer pool write index */
        if (s_pool) {
            buffer_pool_mark_filled(s_pool);
        }
    }
}

#endif /* FW_ENABLE_DMA */
