#ifndef HAL_CLOCK_H
#define HAL_CLOCK_H

#include "sam.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * SAMD51 Clock Configuration HAL
 *
 * Provides two layers:
 *   1. Low-level functions for individual clock subsystems (XOSC32K, DFLL,
 *      DPLLs, GCLK generators, GCLK peripheral channels, MCLK gating).
 *      Each handles its own SYNCBUSY waits and parameter validation.
 *
 *   2. Preset recipes selected by a board-level HAL_CLOCK_PRESET define.
 *      A single call to hal_clock_init_preset() executes the full clock
 *      tree configuration in the correct, validated order.
 *
 * The SAMD51 clock tree is complex (OSCCTRL, GCLK, MCLK, OSC32KCTRL) and
 * init ordering is critical: wrong sequencing, missing sync waits, or
 * invalid DPLL settings can cause hard faults or silent failures. The
 * presets encode the correct ordering learned from the Atmel START
 * reference code (Miniscope-v4-Wire-Free) and the SAMD51 datasheet.
 *
 * SAMD51 boots at 48 MHz DFLL (open-loop) — no clock init is needed for
 * basic operation. This HAL is opt-in: boards that don't define
 * HAL_CLOCK_PRESET simply don't call hal_clock_init_preset().
 *
 * All fatal errors use __BKPT(0) to halt in the debugger, matching the
 * existing HAL convention (see hal_systick.c).
 */

/* ======================================================================
 * Types & Enums
 * ====================================================================== */

/**
 * Clock source selection for GCLK generators.
 * Values map 1:1 to the GCLK_GENCTRL_SRC field in hardware, so the enum
 * value can be written directly into the SRC bitfield.
 */
typedef enum {
    HAL_CLK_SRC_XOSC0    = 0,  /* External crystal oscillator 0 */
    HAL_CLK_SRC_XOSC1    = 1,  /* External crystal oscillator 1 */
    HAL_CLK_SRC_GCLKIN    = 2,  /* External clock input pad */
    HAL_CLK_SRC_GCLKGEN1  = 3,  /* GCLK generator 1 output */
    HAL_CLK_SRC_OSCULP32K = 4,  /* Internal ultra-low-power 32 kHz RC */
    HAL_CLK_SRC_XOSC32K   = 5,  /* External 32.768 kHz crystal */
    HAL_CLK_SRC_DFLL      = 6,  /* DFLL48M output (48 MHz) */
    HAL_CLK_SRC_DPLL0     = 7,  /* DPLL0 output (up to 200 MHz) */
    HAL_CLK_SRC_DPLL1     = 8,  /* DPLL1 output (up to 200 MHz) */
} hal_clk_src_t;

/**
 * DPLL reference clock source selection.
 * Values map 1:1 to OSCCTRL_DPLLCTRLB_REFCLK field values.
 */
typedef enum {
    HAL_DPLL_REF_GCLK    = 0,  /* Dedicated GCLK channel (routed separately) */
    HAL_DPLL_REF_XOSC32K = 1,  /* XOSC32K direct (no GCLK routing needed) */
    HAL_DPLL_REF_XOSC0   = 2,  /* XOSC0 direct */
    HAL_DPLL_REF_XOSC1   = 3,  /* XOSC1 direct */
} hal_dpll_ref_t;

/**
 * DPLL configuration.
 *
 * The HAL computes the LDR (loop divider ratio) and LDRFRAC (fractional
 * part) from ref_hz and output_hz:
 *   output_hz = ref_hz * (LDR + 1 + LDRFRAC/32)
 *
 * If ref_clk is HAL_DPLL_REF_GCLK, the caller must route the appropriate
 * GCLK generator to the DPLL's peripheral channel BEFORE calling
 * hal_clock_enable_dpll(). Use hal_clock_enable_gclk_channel() with
 * OSCCTRL_GCLK_ID_FDPLL0 (for DPLL0) or OSCCTRL_GCLK_ID_FDPLL1 (DPLL1).
 */
typedef struct {
    hal_dpll_ref_t ref_clk;     /* Reference clock source */
    uint32_t       ref_hz;      /* Reference frequency in Hz (for LDR calculation + validation) */
    uint32_t       output_hz;   /* Desired output in Hz (must be 96-200 MHz) */
} hal_dpll_config_t;

/**
 * GCLK generator configuration.
 *
 * The SAMD51 has 12 generators (0-11). Generators 0 and 1 have 16-bit
 * dividers; generators 2-11 have 8-bit dividers (max div = 255).
 */
typedef struct {
    uint8_t         gen;        /* Generator index (0-11) */
    hal_clk_src_t   src;        /* Clock source for this generator */
    uint16_t        div;        /* Division factor (0 or 1 = undivided) */
    bool            idc;        /* Improve duty cycle (useful for odd dividers) */
    bool            oe;         /* Output enable (route clock to GCLK_IO pin) */
} hal_gclk_gen_config_t;

/**
 * XOSC32K (external 32.768 kHz crystal) configuration.
 *
 * The crystal oscillator requires an external 32.768 kHz crystal between
 * the XIN32/XOUT32 pins. Startup time depends on crystal characteristics
 * and board capacitance.
 */
typedef struct {
    uint8_t startup;    /* Startup time select (0-6, maps to 62.6ms - 8000ms) */
    bool    en32k;      /* Enable 32.768 kHz clock output */
    bool    en1k;       /* Enable 1.024 kHz clock output */
    bool    ondemand;   /* On-demand mode (clock only runs when requested) */
    uint8_t cgm;        /* Control gain mode: 1=XT (standard), 2=HS (high-speed) */
} hal_xosc32k_config_t;

/**
 * DFLL closed-loop configuration.
 *
 * In closed-loop mode, the DFLL locks its 48 MHz output to a multiple
 * of a low-frequency reference clock (typically 32.768 kHz from XOSC32K
 * via a GCLK generator).
 *
 * The multiply factor determines the output:
 *   DFLL output = reference_hz * mul
 *   e.g., 32768 Hz * 1465 ≈ 48,005,120 Hz ≈ 48 MHz
 */
typedef struct {
    uint16_t mul;       /* Multiply factor (e.g., 1465 for 48 MHz from 32.768 kHz) */
    uint8_t  cstep;     /* Coarse maximum step size (higher = faster lock, more jitter) */
    uint8_t  fstep;     /* Fine maximum step size */
    uint8_t  gclk_gen;  /* GCLK generator index providing the reference clock */
} hal_dfll_config_t;

/**
 * Clock preset recipes.
 *
 * Each preset configures the complete clock tree in the correct order.
 * Select one via HAL_CLOCK_PRESET in board_config.cmake.
 */
typedef enum {
    /**
     * No-op preset: keeps the default 48 MHz DFLL (open-loop).
     * Only updates the frequency tracking state so hal_clock_get_cpu_freq()
     * returns the correct value. No hardware registers are modified.
     */
    HAL_CLOCK_PRESET_DFLL_48MHZ = 0,

    /**
     * 120 MHz CPU via DPLL0, using DFLL open-loop as reference.
     * No external crystal needed. Chain:
     *   DFLL (48 MHz, open-loop) -> GCLK3 /16 (3 MHz) -> DPLL0 x40 (120 MHz) -> GCLK0
     */
    HAL_CLOCK_PRESET_DPLL_120MHZ,

    /**
     * 120 MHz CPU via DPLL0, with XOSC32K crystal reference.
     * Production-quality chain matching the Miniscope-v4-Wire-Free:
     *   XOSC32K (32.768 kHz) -> GCLK2 -> DFLL closed-loop (48 MHz)
     *     -> GCLK3 /16 (3 MHz) -> DPLL0 x40 (120 MHz) -> GCLK0
     */
    HAL_CLOCK_PRESET_DPLL_120MHZ_XOSC32K,

    /**
     * 60 MHz CPU via DPLL0 with XOSC32K, same chain as 120 MHz but
     * GCLK0 divides DPLL0 by 2.
     *   XOSC32K -> GCLK2 -> DFLL closed-loop (48 MHz)
     *     -> GCLK3 /16 (3 MHz) -> DPLL0 x40 (120 MHz) -> GCLK0 /2 (60 MHz)
     *
     * Used for reduced power consumption or compatibility with the
     * original Wire-Free V4 firmware's 60 MHz CPU clock.
     */
    HAL_CLOCK_PRESET_DPLL_60MHZ_XOSC32K,
} hal_clock_preset_t;

/* ======================================================================
 * Low-Level Functions
 * ====================================================================== */

/**
 * Set NVM flash read wait states for a given CPU frequency.
 *
 * Only needed if AUTOWS (automatic wait states) is disabled, which is
 * unusual — AUTOWS is enabled by default at reset and handles this
 * automatically. The presets verify AUTOWS is enabled rather than
 * calling this function.
 *
 * Wait state table (at 3.3V, from datasheet Table 54-43):
 *   0 WS: <= 24 MHz    3 WS: <= 101 MHz
 *   1 WS: <= 51 MHz    4 WS: <= 119 MHz
 *   2 WS: <= 77 MHz    5 WS: <= 120 MHz
 *
 * @param cpu_hz  Target CPU frequency in Hz.
 */
void hal_clock_set_flash_ws(uint32_t cpu_hz);

/**
 * Enable the XOSC32K external 32.768 kHz crystal oscillator.
 *
 * Configures the oscillator in crystal mode (XTALEN=1) and blocks until
 * the oscillator is stable (STATUS.XOSC32KRDY). Must be called before
 * any clock source that depends on XOSC32K (e.g., DFLL closed-loop).
 *
 * @param config  XOSC32K configuration parameters.
 */
void hal_clock_enable_xosc32k(const hal_xosc32k_config_t *config);

/**
 * Reconfigure the DFLL into closed-loop mode.
 *
 * This function handles the critical GCLK0 temporary switch that is
 * required because GCLK0 sources from DFLL at boot. The sequence is:
 *
 *   1. Route config->gclk_gen to the DFLL peripheral channel
 *   2. Temporarily switch GCLK0 to OSCULP32K (CPU runs at ~32 kHz!)
 *   3. Disable DFLL, reconfigure for closed-loop, re-enable
 *   4. Wait for DFLL lock (DFLLRDY + DFLLLCKC)
 *
 * IMPORTANT: After this function returns, GCLK0 is still on OSCULP32K.
 * The caller must switch GCLK0 to its final source (e.g., DPLL0).
 * The CPU will be running very slowly (~32 kHz) until that switch.
 *
 * @param config  DFLL closed-loop configuration parameters.
 */
void hal_clock_configure_dfll_closed_loop(const hal_dfll_config_t *config);

/**
 * Configure and enable a DPLL (Digital Phase-Locked Loop).
 *
 * Computes LDR and LDRFRAC from the requested output and reference
 * frequencies, enables the DPLL, and waits for lock (LOCK || CLKRDY).
 *
 * Validates:
 *   - index is 0 or 1
 *   - output_hz is in [96 MHz, 200 MHz]
 *   - ref_hz is in [32 kHz, 3.2 MHz]
 *
 * If ref_clk is HAL_DPLL_REF_GCLK, the caller must have already routed
 * the GCLK generator to the DPLL's peripheral channel.
 *
 * @param index   DPLL index (0 or 1).
 * @param config  DPLL configuration parameters.
 */
void hal_clock_enable_dpll(uint8_t index, const hal_dpll_config_t *config);

/**
 * Configure a GCLK generator.
 *
 * Sets the source, divider, and options for a generator, enables it,
 * and waits for SYNCBUSY to clear. Also updates internal frequency
 * tracking if a frequency can be determined.
 *
 * Validates:
 *   - gen index is 0-11
 *   - div fits in the generator's divider width (8-bit for gen 2-11)
 *
 * @param config  Generator configuration parameters.
 */
void hal_clock_configure_gclk_gen(const hal_gclk_gen_config_t *config);

/**
 * Route a GCLK generator to a peripheral channel.
 *
 * Each peripheral has a dedicated channel ID (e.g., OSCCTRL_GCLK_ID_DFLL48,
 * SERCOM0_GCLK_ID_CORE). The DFP headers provide these as *_GCLK_ID_*
 * defines.
 *
 * @param channel_id  Peripheral channel index (0-47).
 * @param gen         GCLK generator index (0-11) to route to this channel.
 */
void hal_clock_enable_gclk_channel(uint8_t channel_id, uint8_t gen);

/**
 * Disable a GCLK peripheral channel.
 *
 * @param channel_id  Peripheral channel index (0-47).
 */
void hal_clock_disable_gclk_channel(uint8_t channel_id);

/* ======================================================================
 * MCLK Bus Clock Gating (inline, zero overhead)
 *
 * MCLK controls which peripherals receive a bus clock. Peripherals must
 * be unmasked (enabled) before their registers can be accessed.
 *
 * Usage example:
 *   hal_clock_enable_apb(&MCLK_REGS->MCLK_APBAMASK, MCLK_APBAMASK_SERCOM0_Msk);
 *
 * The DFP provides all MCLK_APBx_*_Msk defines.
 * ====================================================================== */

/** Enable a peripheral's bus clock by setting its mask bit. */
static inline void hal_clock_enable_apb(volatile uint32_t *mask_reg, uint32_t mask)
{
    *mask_reg |= mask;
}

/** Disable a peripheral's bus clock by clearing its mask bit. */
static inline void hal_clock_disable_apb(volatile uint32_t *mask_reg, uint32_t mask)
{
    *mask_reg &= ~mask;
}

/* ======================================================================
 * Frequency Tracking
 *
 * Presets and low-level functions update an internal table of GCLK
 * generator frequencies. Drivers can query these at runtime instead of
 * relying on compile-time F_CPU.
 * ====================================================================== */

/** Return the current CPU clock frequency in Hz (GCLK0 output). */
uint32_t hal_clock_get_cpu_freq(void);

/**
 * Return the current output frequency of a GCLK generator in Hz.
 *
 * @param gen  Generator index (0-11). Returns 0 for out-of-range.
 */
uint32_t hal_clock_get_gclk_freq(uint8_t gen);

/* ======================================================================
 * Preset Interface
 * ====================================================================== */

/**
 * Initialize the entire clock tree using a preset recipe.
 *
 * This is the primary board-level interface. Call once from board_init()
 * before any peripheral or timer initialization. The preset handles all
 * oscillator startup, GCLK routing, DFLL/DPLL configuration, and sync
 * waits in the correct order.
 *
 * @param preset  Clock configuration recipe to apply.
 */
void hal_clock_init_preset(hal_clock_preset_t preset);

#endif /* HAL_CLOCK_H */
