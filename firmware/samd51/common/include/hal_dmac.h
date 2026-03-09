#ifndef HAL_DMAC_H
#define HAL_DMAC_H

#include "sam.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * Thin DMAC helpers for SAMD51.
 * Provides descriptor array declaration, one-time init, descriptor fill
 * with end-of-buffer address math, and linked-list chaining.
 *
 * Channel config, interrupts, and triggers are left to the caller
 * using DFP v3.x register defines directly.
 */

/**
 * Declare a descriptor array for use as BASEADDR or WRBADDR target.
 * Sized to DMAC_CH_NUM (32) entries — one per channel.
 * Placed in .hsram and aligned to 16 bytes.
 */
#define DMAC_DESCRIPTOR_ARRAY(name) \
    SECTION_DMAC_DESCRIPTOR static dmac_descriptor_registers_t \
    name[DMAC_CH_NUM] __attribute__((aligned(16)))

/**
 * Inputs for populating a DMA transfer descriptor.
 * The caller builds btctrl from DFP DMAC_BTCTRL_* defines.
 */
typedef struct {
    const void *src_addr;     /* Source buffer start (or peripheral register) */
    void       *dst_addr;     /* Destination buffer start (or peripheral register) */
    uint16_t    beat_count;   /* Number of beats in block transfer */
    uint16_t    btctrl;       /* BTCTRL value — caller builds from DFP defines */
    dmac_descriptor_registers_t *next;  /* Next descriptor (NULL = end of list) */
} dmac_descriptor_config_t;

/**
 * One-time DMAC init: reset, set descriptor base addresses, enable.
 * All four priority levels are enabled.
 */
static inline void dmac_init(
    dmac_descriptor_registers_t *base_descriptors,
    dmac_descriptor_registers_t *writeback_descriptors)
{
    /* Software reset */
    DMAC_REGS->DMAC_CTRL = DMAC_CTRL_SWRST_Msk;
    while (DMAC_REGS->DMAC_CTRL & DMAC_CTRL_SWRST_Msk) {
        /* wait for reset to complete */
    }

    /* Set descriptor table addresses */
    DMAC_REGS->DMAC_BASEADDR = (uint32_t)base_descriptors;
    DMAC_REGS->DMAC_WRBADDR  = (uint32_t)writeback_descriptors;

    /* Enable DMAC with all priority levels */
    DMAC_REGS->DMAC_CTRL =
        DMAC_CTRL_DMAENABLE_Msk
        | DMAC_CTRL_LVLEN0_Msk
        | DMAC_CTRL_LVLEN1_Msk
        | DMAC_CTRL_LVLEN2_Msk
        | DMAC_CTRL_LVLEN3_Msk;
}

/**
 * Populate a descriptor from a config struct.
 *
 * Handles the SAMD51 address convention: when SRCINC or DSTINC is set,
 * the hardware wants the address of one past the last beat, not the
 * buffer start. Step size is factored in when STEPSEL matches.
 */
static inline void dmac_descriptor_fill(
    dmac_descriptor_registers_t *desc,
    const dmac_descriptor_config_t *cfg)
{
    uint16_t btctrl = cfg->btctrl;

    /* Beat size in bytes: 1, 2, or 4 */
    uint32_t beat_bytes =
        1u << ((btctrl & DMAC_BTCTRL_BEATSIZE_Msk) >> DMAC_BTCTRL_BEATSIZE_Pos);

    /* Step multiplier: 1, 2, 4, ... 128 (from STEPSIZE field) */
    uint32_t step_mul =
        1u << ((btctrl & DMAC_BTCTRL_STEPSIZE_Msk) >> DMAC_BTCTRL_STEPSIZE_Pos);

    /* STEPSEL: 0 = step applies to DST, 1 = step applies to SRC */
    bool step_applies_to_src = (btctrl & DMAC_BTCTRL_STEPSEL_Msk) != 0;

    /* Source address — if incrementing, hardware wants end-of-buffer */
    uint32_t src = (uint32_t)cfg->src_addr;
    if (btctrl & DMAC_BTCTRL_SRCINC_Msk) {
        uint32_t stride = beat_bytes * (step_applies_to_src ? step_mul : 1u);
        src += (uint32_t)cfg->beat_count * stride;
    }

    /* Destination address — same end-of-buffer rule */
    uint32_t dst = (uint32_t)cfg->dst_addr;
    if (btctrl & DMAC_BTCTRL_DSTINC_Msk) {
        uint32_t stride = beat_bytes * (step_applies_to_src ? 1u : step_mul);
        dst += (uint32_t)cfg->beat_count * stride;
    }

    desc->DMAC_BTCTRL  = btctrl;
    desc->DMAC_BTCNT   = cfg->beat_count;
    desc->DMAC_SRCADDR = src;
    desc->DMAC_DSTADDR = dst;
    desc->DMAC_DESCADDR = (uint32_t)cfg->next;
}

/**
 * Chain an array of descriptors into a linked list.
 * If circular, the last descriptor links back to the first.
 */
static inline void dmac_link_descriptors(
    dmac_descriptor_registers_t *descs,
    uint32_t count,
    bool circular)
{
    if (count == 0) {
        __BKPT(0);  /* count must be at least 1 */
        return;
    }

    for (uint32_t i = 0; i < count - 1u; i++) {
        descs[i].DMAC_DESCADDR = (uint32_t)&descs[i + 1u];
    }
    descs[count - 1u].DMAC_DESCADDR = circular ? (uint32_t)&descs[0] : 0u;
}

#endif /* HAL_DMAC_H */
