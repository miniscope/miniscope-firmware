#include "hal_clock.h"

/* ======================================================================
 * Internal Constants
 * ====================================================================== */

/**
 * Timeout for synchronization waits (SYNCBUSY bit polling).
 *
 * Sync operations complete within a few clock cycles of the slower domain.
 * During DFLL reconfiguration, GCLK0 is temporarily on OSCULP32K (~32 kHz),
 * where each loop iteration takes ~100-150 us. At 10000 iterations this
 * gives ~1-1.5 seconds — far more than any sync should take.
 */
#define SYNC_WAIT_TIMEOUT   10000U

/**
 * Timeout for oscillator lock waits (DFLL lock, DPLL lock).
 *
 * DPLL lock typically takes a few hundred microseconds. DFLL closed-loop
 * lock depends on step sizes and can take longer. At ~32 kHz CPU (worst
 * case during DFLL reconfig), 100000 iterations ≈ 10-15 seconds.
 */
#define LOCK_WAIT_TIMEOUT   100000U

/** SAMD51 has 12 GCLK generators (indices 0-11). */
#define GCLK_GEN_COUNT      12U

/** SAMD51 has 48 GCLK peripheral channels (indices 0-47). */
#define GCLK_CHAN_COUNT      48U

/** SAMD51 has 2 DPLLs (indices 0, 1). */
#define DPLL_COUNT           2U

/** DPLL output frequency limits (from datasheet). */
#define DPLL_MIN_OUTPUT_HZ  96000000U
#define DPLL_MAX_OUTPUT_HZ  200000000U

/** DPLL reference frequency limits (from datasheet). */
#define DPLL_MIN_REF_HZ     32000U
#define DPLL_MAX_REF_HZ     3200000U

/** DPLL LDR (loop divider ratio) is 13-bit, max value 8191. */
#define DPLL_LDR_MAX         0x1FFFU

/** DPLL LDRFRAC fractional divisor (output = ref * (LDR + 1 + LDRFRAC/32)). */
#define DPLL_LDRFRAC_DIV     32U

/** GCLK generators 2-11 have 8-bit dividers (max 255). */
#define GCLK_DIV8_MAX        255U

/** DFLL nominal output frequency. */
#define DFLL_OUTPUT_HZ       48000000U

/* ======================================================================
 * Internal Sync/Status Wait Helpers
 *
 * All hardware waits are bounded to prevent infinite hangs from
 * misconfiguration. Timeout triggers __BKPT(0) to halt in the debugger.
 * ====================================================================== */

/**
 * Wait for bits in a 32-bit register to CLEAR (sync complete).
 * Used for GCLK_SYNCBUSY, DPLL_SYNCBUSY, etc.
 */
static void sync_wait_32(volatile const uint32_t *reg, uint32_t mask)
{
    uint32_t timeout = SYNC_WAIT_TIMEOUT;
    while ((*reg & mask) && --timeout) {
        /* Spin until sync bits clear or timeout */
    }
    if (!timeout) {
        __BKPT(0);  /* sync timed out — hardware misconfiguration */
    }
}

/**
 * Wait for bits in an 8-bit register to CLEAR (sync complete).
 * Used for OSCCTRL_DFLLSYNC, which is an 8-bit register.
 */
static void sync_wait_8(volatile const uint8_t *reg, uint8_t mask)
{
    uint32_t timeout = SYNC_WAIT_TIMEOUT;
    while ((*reg & mask) && --timeout) {
        /* Spin until sync bits clear or timeout */
    }
    if (!timeout) {
        __BKPT(0);  /* sync timed out — hardware misconfiguration */
    }
}

/**
 * Wait for ALL specified bits in a 32-bit register to SET (AND condition).
 * Used for DFLL lock: both DFLLRDY and DFLLLCKC must be set.
 */
static void status_wait_all(volatile const uint32_t *reg, uint32_t mask)
{
    uint32_t timeout = LOCK_WAIT_TIMEOUT;
    while ((*reg & mask) != mask && --timeout) {
        /* Spin until all required status bits are set or timeout */
    }
    if (!timeout) {
        __BKPT(0);  /* oscillator lock timed out */
    }
}

/**
 * Wait for ANY of the specified bits in a 32-bit register to SET (OR condition).
 * Used for DPLL lock: either LOCK or CLKRDY indicates the DPLL is usable.
 * This matches the Atmel START behavior and datasheet recommendation.
 */
static void status_wait_any(volatile const uint32_t *reg, uint32_t mask)
{
    uint32_t timeout = LOCK_WAIT_TIMEOUT;
    while (!(*reg & mask) && --timeout) {
        /* Spin until any status bit is set or timeout */
    }
    if (!timeout) {
        __BKPT(0);  /* oscillator lock timed out */
    }
}

/* ======================================================================
 * Frequency Tracking State
 * ====================================================================== */

/**
 * Tracked output frequency (in Hz) for each GCLK generator.
 *
 * Updated by hal_clock_configure_gclk_gen() and the preset functions.
 * Index 0 = GCLK0 = CPU clock (always routed to the CPU via MCLK).
 * Initialized to 0; preset functions set correct values.
 */
static uint32_t s_gclk_freq[GCLK_GEN_COUNT];

/* ======================================================================
 * Low-Level Function Implementations
 * ====================================================================== */

void hal_clock_set_flash_ws(uint32_t cpu_hz)
{
    /*
     * NVM read wait states at 3.3V (SAMD51 datasheet Table 54-43).
     * Only needed if AUTOWS is disabled — AUTOWS handles this automatically
     * and is enabled by default at reset.
     */
    uint8_t ws;
    if      (cpu_hz <= 24000000U)  ws = 0;
    else if (cpu_hz <= 51000000U)  ws = 1;
    else if (cpu_hz <= 77000000U)  ws = 2;
    else if (cpu_hz <= 101000000U) ws = 3;
    else if (cpu_hz <= 119000000U) ws = 4;
    else                           ws = 5;

    /* Read-modify-write to preserve other CTRLA fields (WMODE, AUTOWS, etc.) */
    NVMCTRL_REGS->NVMCTRL_CTRLA =
        (NVMCTRL_REGS->NVMCTRL_CTRLA & ~NVMCTRL_CTRLA_RWS_Msk)
        | NVMCTRL_CTRLA_RWS(ws);
}

void hal_clock_enable_xosc32k(const hal_xosc32k_config_t *config)
{
    /*
     * Configure the external 32.768 kHz crystal oscillator (XOSC32K).
     *
     * The crystal must be connected between XIN32 and XOUT32 pins with
     * appropriate load capacitors. XTALEN=1 selects crystal mode (vs.
     * external clock input mode).
     *
     * CGM (Control Gain Mode) should match the crystal:
     *   1 = XT (standard gain, for typical watch crystals)
     *   2 = HS (high-speed, for higher-frequency or high-ESR crystals)
     *
     * ONDEMAND is initially cleared so the oscillator starts immediately.
     * If config->ondemand is true, we set it after the oscillator is ready
     * (same pattern as the Atmel START reference code).
     */
    uint16_t reg = OSC32KCTRL_XOSC32K_STARTUP(config->startup)
                 | OSC32KCTRL_XOSC32K_CGM(config->cgm)
                 | OSC32KCTRL_XOSC32K_XTALEN_Msk   /* crystal mode */
                 | OSC32KCTRL_XOSC32K_ENABLE_Msk;   /* enable oscillator */

    if (config->en32k) {
        reg |= OSC32KCTRL_XOSC32K_EN32K_Msk;
    }
    if (config->en1k) {
        reg |= OSC32KCTRL_XOSC32K_EN1K_Msk;
    }
    /* Note: ONDEMAND is NOT set here — we need the oscillator running
     * immediately so we can wait for it. Set after ready if requested. */

    OSC32KCTRL_REGS->OSC32KCTRL_XOSC32K = reg;

    /* Block until the oscillator is stable and producing a valid clock. */
    status_wait_all(&OSC32KCTRL_REGS->OSC32KCTRL_STATUS,
                    OSC32KCTRL_STATUS_XOSC32KRDY_Msk);

    /* Now safe to enable on-demand mode if requested. In on-demand mode
     * the oscillator only runs when a peripheral actually needs the clock,
     * saving power in sleep modes. */
    if (config->ondemand) {
        OSC32KCTRL_REGS->OSC32KCTRL_XOSC32K |= OSC32KCTRL_XOSC32K_ONDEMAND_Msk;
    }
}

void hal_clock_configure_dfll_closed_loop(const hal_dfll_config_t *config)
{
    /*
     * Reconfigure the DFLL from open-loop (boot default) to closed-loop.
     *
     * CRITICAL SEQUENCING: At boot, GCLK0 (the CPU clock) sources from
     * the DFLL. We cannot reconfigure the DFLL while GCLK0 is using it
     * as a source — the CPU would lose its clock. The solution (from the
     * Atmel START reference code in hpl_oscctrl.c) is:
     *
     *   1. Temporarily switch GCLK0 to the ultra-low-power 32 kHz RC
     *      (OSCULP32K). The CPU continues running, but very slowly.
     *   2. Disable and reconfigure the DFLL.
     *   3. Wait for the DFLL to lock.
     *   4. The CALLER switches GCLK0 to its final source (e.g., DPLL0).
     *
     * After this function returns, GCLK0 is STILL on OSCULP32K (~32 kHz).
     * The caller MUST switch GCLK0 to the final clock source.
     */

    /* --- Validate parameters --- */
    if (config->gclk_gen >= GCLK_GEN_COUNT) {
        __BKPT(0);  /* GCLK generator index out of range (0-11) */
        return;
    }

    /* --- Step 1: Temporarily switch GCLK0 away from DFLL ---
     *
     * OSCULP32K is always-on and needs no setup, making it a safe
     * temporary source. The CPU will run at ~32 kHz until the caller
     * sets GCLK0 to the final source (typically DPLL0 at 120 MHz).
     */
    GCLK_REGS->GCLK_GENCTRL[0] =
        (GCLK_REGS->GCLK_GENCTRL[0] & ~GCLK_GENCTRL_SRC_Msk)
        | GCLK_GENCTRL_SRC(HAL_CLK_SRC_OSCULP32K);

    /* Wait for the GCLK0 source switch to synchronize. The SYNCBUSY bit
     * for generator N is at position (GCLK_SYNCBUSY_GENCTRL_Pos + N). */
    sync_wait_32(&GCLK_REGS->GCLK_SYNCBUSY,
                 1u << (GCLK_SYNCBUSY_GENCTRL_Pos + 0));

    /* --- Step 2: Disable the DFLL ---
     *
     * Must be disabled before changing MUL or mode. Writing 0 to CTRLA
     * clears ENABLE and all other bits. */
    OSCCTRL_REGS->OSCCTRL_DFLLCTRLA = 0;

    /* --- Step 3: Route GCLK reference to the DFLL peripheral channel ---
     *
     * The DFLL's reference input is GCLK peripheral channel 0
     * (OSCCTRL_GCLK_ID_DFLL48). We route the specified generator to it.
     * This must happen after DFLL is disabled (can't change reference
     * while DFLL is running in closed-loop). */
    GCLK_REGS->GCLK_PCHCTRL[OSCCTRL_GCLK_ID_DFLL48] =
        GCLK_PCHCTRL_GEN(config->gclk_gen) | GCLK_PCHCTRL_CHEN_Msk;

    /* --- Step 4: Configure DFLL multiply factor ---
     *
     * DFLLMUL sets how the DFLL locks in closed-loop:
     *   MUL: multiplication factor (output = reference * MUL)
     *   FSTEP: max fine adjustment step per reference clock cycle
     *   CSTEP: max coarse adjustment step per reference clock cycle
     *
     * Larger step values → faster lock but more output jitter.
     * Typical values from Wire-Free reference: MUL=1465, CSTEP=0x1f, FSTEP=0x7f
     */
    OSCCTRL_REGS->OSCCTRL_DFLLMUL =
        OSCCTRL_DFLLMUL_MUL(config->mul)
        | OSCCTRL_DFLLMUL_FSTEP(config->fstep)
        | OSCCTRL_DFLLMUL_CSTEP(config->cstep);

    /* Wait for DFLLMUL write to synchronize to the DFLL clock domain. */
    sync_wait_8(&OSCCTRL_REGS->OSCCTRL_DFLLSYNC, OSCCTRL_DFLLSYNC_DFLLMUL_Msk);

    /* --- Step 5: Clear DFLLCTRLB (required before enable) ---
     *
     * The Atmel START reference writes DFLLCTRLB=0 first, then enables
     * the DFLL, then writes DFLLCTRLB again with the actual mode. This
     * two-phase approach ensures the DFLL starts in open-loop before
     * being switched to closed-loop, which is required by the hardware
     * sequencing rules in the datasheet. */
    OSCCTRL_REGS->OSCCTRL_DFLLCTRLB = 0;
    sync_wait_8(&OSCCTRL_REGS->OSCCTRL_DFLLSYNC, OSCCTRL_DFLLSYNC_DFLLCTRLB_Msk);

    /* --- Step 6: Enable the DFLL (initially in open-loop) ---
     *
     * ONDEMAND is cleared so the DFLL runs continuously.
     * RUNSTDBY is not set (DFLL stops in standby to save power). */
    OSCCTRL_REGS->OSCCTRL_DFLLCTRLA = OSCCTRL_DFLLCTRLA_ENABLE_Msk;
    sync_wait_8(&OSCCTRL_REGS->OSCCTRL_DFLLSYNC, OSCCTRL_DFLLSYNC_ENABLE_Msk);

    /* --- Step 7: Latch DFLLVAL ---
     *
     * Read-modify-write of DFLLVAL to latch the current calibration
     * values. This is done by the Atmel START reference code after enable
     * and is required for correct DFLL operation per the errata. */
    OSCCTRL_REGS->OSCCTRL_DFLLVAL = OSCCTRL_REGS->OSCCTRL_DFLLVAL;
    sync_wait_8(&OSCCTRL_REGS->OSCCTRL_DFLLSYNC, OSCCTRL_DFLLSYNC_DFLLVAL_Msk);

    /* --- Step 8: Switch to closed-loop mode ---
     *
     * MODE=1 enables closed-loop tracking against the GCLK reference.
     * WAITLOCK=1 gates the output until the DFLL is locked, preventing
     * downstream PLLs from seeing an unlocked intermediate frequency.
     *
     * The DFLL will now begin tracking the reference and adjusting its
     * frequency to match reference * MUL. */
    OSCCTRL_REGS->OSCCTRL_DFLLCTRLB =
        OSCCTRL_DFLLCTRLB_MODE_Msk | OSCCTRL_DFLLCTRLB_WAITLOCK_Msk;
    sync_wait_8(&OSCCTRL_REGS->OSCCTRL_DFLLSYNC, OSCCTRL_DFLLSYNC_DFLLCTRLB_Msk);

    /* --- Step 9: Wait for DFLL lock ---
     *
     * In closed-loop mode we wait for both:
     *   DFLLRDY:  DFLL output is ready
     *   DFLLLCKC: Coarse lock achieved
     *
     * This matches the Atmel START reference (hpl_oscctrl.c lines 188-196)
     * which checks (DFLLRDY | DFLLLCKC) == (DFLLRDY | DFLLLCKC). */
    status_wait_all(&OSCCTRL_REGS->OSCCTRL_STATUS,
                    OSCCTRL_STATUS_DFLLRDY_Msk | OSCCTRL_STATUS_DFLLLCKC_Msk);
}

void hal_clock_enable_dpll(uint8_t index, const hal_dpll_config_t *config)
{
    /* --- Validate parameters --- */

    if (index >= DPLL_COUNT) {
        __BKPT(0);  /* invalid DPLL index (only 0 and 1 exist) */
        return;
    }
    if (config->output_hz < DPLL_MIN_OUTPUT_HZ ||
        config->output_hz > DPLL_MAX_OUTPUT_HZ) {
        __BKPT(0);  /* DPLL output must be 96-200 MHz */
        return;
    }
    if (config->ref_hz < DPLL_MIN_REF_HZ ||
        config->ref_hz > DPLL_MAX_REF_HZ) {
        __BKPT(0);  /* DPLL reference must be 32 kHz - 3.2 MHz */
        return;
    }

    /*
     * Compute the Loop Divider Ratio (LDR) and fractional part (LDRFRAC).
     *
     * The DPLL output frequency is:
     *   f_out = f_ref * (LDR + 1 + LDRFRAC/32)
     *
     * Solving for LDR and LDRFRAC:
     *   ratio     = f_out / f_ref                    (integer part)
     *   LDR       = ratio - 1
     *   remainder = f_out - f_ref * ratio            (what's left)
     *   LDRFRAC   = (remainder * 32) / f_ref         (fractional 1/32 steps)
     *
     * Example: f_out=120MHz, f_ref=3MHz
     *   ratio=40, LDR=39, remainder=0, LDRFRAC=0
     *   Check: 3MHz * (39 + 1 + 0/32) = 120 MHz ✓
     */
    uint32_t ratio     = config->output_hz / config->ref_hz;
    uint32_t ldr       = ratio - 1;
    uint32_t remainder = config->output_hz - (config->ref_hz * ratio);
    uint32_t ldrfrac   = (remainder * DPLL_LDRFRAC_DIV) / config->ref_hz;

    /* LDR is 13-bit (max 8191). If we exceed this, the configuration
     * is invalid (reference frequency too low for the desired output). */
    if (ldr > DPLL_LDR_MAX) {
        __BKPT(0);  /* LDR overflow — ref_hz too low for output_hz */
        return;
    }

    /* --- Configure DPLL ratio register ---
     *
     * Must be written before enabling the DPLL. The DPLLRATIO register
     * requires synchronization — wait for DPLLSYNCBUSY.DPLLRATIO to clear. */
    OSCCTRL_REGS->DPLL[index].OSCCTRL_DPLLRATIO =
        OSCCTRL_DPLLRATIO_LDR(ldr) | OSCCTRL_DPLLRATIO_LDRFRAC(ldrfrac);

    sync_wait_32(&OSCCTRL_REGS->DPLL[index].OSCCTRL_DPLLSYNCBUSY,
                 OSCCTRL_DPLLSYNCBUSY_DPLLRATIO_Msk);

    /* --- Configure DPLL control B (reference clock selection) ---
     *
     * REFCLK selects where the DPLL gets its reference:
     *   0 = GCLK (dedicated peripheral channel, must be routed by caller)
     *   1 = XOSC32K (direct, no GCLK routing needed)
     *   2 = XOSC0, 3 = XOSC1
     *
     * All other fields (FILTER, LTIME, LBYPASS, etc.) are left at defaults
     * (0), which gives standard lock behavior suitable for most uses. */
    OSCCTRL_REGS->DPLL[index].OSCCTRL_DPLLCTRLB =
        OSCCTRL_DPLLCTRLB_REFCLK(config->ref_clk);

    /* --- Enable the DPLL ---
     *
     * ONDEMAND is cleared so the DPLL runs continuously.
     * After writing ENABLE, wait for the sync to complete. */
    OSCCTRL_REGS->DPLL[index].OSCCTRL_DPLLCTRLA = OSCCTRL_DPLLCTRLA_ENABLE_Msk;

    sync_wait_32(&OSCCTRL_REGS->DPLL[index].OSCCTRL_DPLLSYNCBUSY,
                 OSCCTRL_DPLLSYNCBUSY_ENABLE_Msk);

    /* --- Wait for DPLL lock ---
     *
     * The DPLL is usable once either LOCK or CLKRDY is set (OR condition).
     * This matches the Atmel START reference (hpl_oscctrl.c lines 202-206)
     * and the datasheet recommendation. The OR condition is used because
     * LOCK may briefly deassert during normal jitter while CLKRDY remains
     * asserted (output is still valid). */
    status_wait_any(
        &OSCCTRL_REGS->DPLL[index].OSCCTRL_DPLLSTATUS,
        OSCCTRL_DPLLSTATUS_LOCK_Msk | OSCCTRL_DPLLSTATUS_CLKRDY_Msk);
}

void hal_clock_configure_gclk_gen(const hal_gclk_gen_config_t *config)
{
    /* --- Validate parameters --- */

    if (config->gen >= GCLK_GEN_COUNT) {
        __BKPT(0);  /* generator index out of range (0-11) */
        return;
    }

    /*
     * Generators 0 and 1 have 16-bit dividers (max 65535).
     * Generators 2-11 have 8-bit dividers (max 255).
     * A div value of 0 or 1 means undivided (no division).
     */
    if (config->gen >= 2 && config->div > GCLK_DIV8_MAX) {
        __BKPT(0);  /* 8-bit divider overflow for generators 2-11 */
        return;
    }

    /*
     * Build the GENCTRL register value:
     *   SRC:   clock source (maps directly from hal_clk_src_t)
     *   GENEN: generator enable
     *   IDC:   improve duty cycle (adds a half-cycle delay for even duty)
     *   DIV:   division factor (bits [31:16])
     */
    uint32_t genctrl = GCLK_GENCTRL_SRC(config->src)
                     | GCLK_GENCTRL_GENEN_Msk
                     | GCLK_GENCTRL_DIV(config->div);

    if (config->idc) {
        genctrl |= GCLK_GENCTRL_IDC_Msk;
    }
    if (config->oe) {
        genctrl |= GCLK_GENCTRL_OE_Msk;
    }

    /* Write the complete GENCTRL register. The GCLK peripheral requires
     * the full register to be written in a single operation (no RMW). */
    GCLK_REGS->GCLK_GENCTRL[config->gen] = genctrl;

    /* Wait for the generator configuration to synchronize. Each generator
     * has its own SYNCBUSY bit at position (GENCTRL_Pos + gen_index). */
    sync_wait_32(&GCLK_REGS->GCLK_SYNCBUSY,
                 1u << (GCLK_SYNCBUSY_GENCTRL_Pos + config->gen));
}

void hal_clock_enable_gclk_channel(uint8_t channel_id, uint8_t gen)
{
    if (channel_id >= GCLK_CHAN_COUNT) {
        __BKPT(0);  /* peripheral channel index out of range (0-47) */
        return;
    }
    if (gen >= GCLK_GEN_COUNT) {
        __BKPT(0);  /* generator index out of range (0-11) */
        return;
    }

    /*
     * Route the specified generator to this peripheral channel and enable it.
     * PCHCTRL is not synchronized — the write takes effect immediately.
     */
    GCLK_REGS->GCLK_PCHCTRL[channel_id] =
        GCLK_PCHCTRL_GEN(gen) | GCLK_PCHCTRL_CHEN_Msk;
}

void hal_clock_disable_gclk_channel(uint8_t channel_id)
{
    if (channel_id >= GCLK_CHAN_COUNT) {
        __BKPT(0);  /* peripheral channel index out of range (0-47) */
        return;
    }

    /* Clearing CHEN disables the channel. GEN field doesn't matter when
     * the channel is disabled, but writing 0 keeps the register clean. */
    GCLK_REGS->GCLK_PCHCTRL[channel_id] = 0;
}

/* ======================================================================
 * Frequency Tracking
 * ====================================================================== */

uint32_t hal_clock_get_cpu_freq(void)
{
    /* GCLK0 is always routed to the CPU (via MCLK). The CPU frequency
     * is GCLK0 output divided by MCLK_CPUDIV (which defaults to 1). */
    return s_gclk_freq[0];
}

uint32_t hal_clock_get_gclk_freq(uint8_t gen)
{
    if (gen >= GCLK_GEN_COUNT) {
        return 0;
    }
    return s_gclk_freq[gen];
}

/* ======================================================================
 * Preset Implementations
 *
 * IMPORTANT: Each preset is intentionally self-contained, even if presets
 * share similar setup steps.  Clock init ordering is safety-critical and
 * presets may diverge independently over time (different oscillator
 * parameters, different DPLL configs, etc.).  Do NOT refactor common
 * steps into a shared helper — keep each preset fully readable in
 * isolation.
 * ====================================================================== */

/**
 * Verify that AUTOWS (automatic NVM wait states) is enabled.
 *
 * AUTOWS is enabled at reset and automatically inserts the correct number
 * of NVM read wait states based on the CPU frequency. This eliminates the
 * need to manually track and update wait states during clock changes.
 *
 * If AUTOWS is disabled (unusual — only if someone explicitly cleared it),
 * we halt because running at 120 MHz with insufficient wait states would
 * cause NVM read errors and unpredictable behavior.
 */
static void verify_autows(void)
{
    if (!(NVMCTRL_REGS->NVMCTRL_CTRLA & NVMCTRL_CTRLA_AUTOWS_Msk)) {
        __BKPT(0);  /* AUTOWS must be enabled for safe frequency scaling */
    }
}

/**
 * Preset: HAL_CLOCK_PRESET_DFLL_48MHZ
 *
 * No-op — the SAMD51 boots at 48 MHz DFLL in open-loop mode and no
 * hardware reconfiguration is needed. We just record the frequency so
 * hal_clock_get_cpu_freq() returns the correct value.
 */
static void preset_dfll_48mhz(void)
{
    /* GCLK0 is already configured by hardware reset:
     *   Source: DFLL (48 MHz, open-loop)
     *   Divider: 1 (undivided)
     * Just update tracking. */
    s_gclk_freq[0] = DFLL_OUTPUT_HZ;
}

/**
 * Preset: HAL_CLOCK_PRESET_DPLL_120MHZ
 *
 * 120 MHz CPU via DPLL0, using the boot-default DFLL (48 MHz, open-loop)
 * as the reference. No external crystal needed.
 *
 * Clock chain:
 *   DFLL (48 MHz, open-loop) → GCLK3 ÷16 (3 MHz) → DPLL0 ×40 (120 MHz) → GCLK0
 *
 * Sequence:
 *   1. Verify AUTOWS (NVM auto wait states)
 *   2. Configure GCLK3 = DFLL ÷16 = 3 MHz (DPLL0 reference)
 *   3. Route GCLK3 → DPLL0 peripheral channel
 *   4. Configure DPLL0: 3 MHz × 40 = 120 MHz
 *   5. Wait for DPLL0 lock
 *   6. Switch GCLK0 to DPLL0 (CPU jumps to 120 MHz)
 *   7. Update frequency tracking
 *
 * Why GCLK3 ÷16?
 *   The DPLL reference input must be 32 kHz - 3.2 MHz (datasheet limit).
 *   48 MHz ÷ 16 = 3 MHz, which is within range and gives a clean
 *   integer multiply to 120 MHz (LDR=39, LDRFRAC=0).
 */
static void preset_dpll_120mhz(void)
{
    verify_autows();

    /* Step 1: GCLK3 = DFLL ÷ 16 = 3 MHz
     * This provides the reference clock for DPLL0.
     * Using GCLK3 follows the Atmel START convention (Wire-Free reference). */
    hal_gclk_gen_config_t gclk3_cfg = {
        .gen = 3,
        .src = HAL_CLK_SRC_DFLL,   /* 48 MHz DFLL (still open-loop) */
        .div = 16,                  /* 48 MHz / 16 = 3 MHz */
        .idc = false,               /* Even divider, duty cycle is fine */
    };
    hal_clock_configure_gclk_gen(&gclk3_cfg);
    s_gclk_freq[3] = DFLL_OUTPUT_HZ / 16U;  /* 3 MHz */

    /* Step 2: Route GCLK3 to DPLL0's peripheral channel.
     * OSCCTRL_GCLK_ID_FDPLL0 (= 1) is the GCLK channel that feeds DPLL0
     * when DPLL0's REFCLK is set to GCLK. */
    hal_clock_enable_gclk_channel(OSCCTRL_GCLK_ID_FDPLL0, 3);

    /* Step 3: Configure and enable DPLL0.
     * 3 MHz × (39 + 1) = 120 MHz. REFCLK=GCLK routes from the channel
     * we just configured above. */
    hal_dpll_config_t dpll0_cfg = {
        .ref_clk   = HAL_DPLL_REF_GCLK,
        .ref_hz    = 3000000U,          /* 3 MHz from GCLK3 */
        .output_hz = 120000000U,        /* 120 MHz target */
    };
    hal_clock_enable_dpll(0, &dpll0_cfg);

    /* Step 4: Switch GCLK0 to DPLL0.
     * This is the moment the CPU frequency jumps from 48 MHz to 120 MHz.
     * IDC=1 improves the duty cycle of the output clock. */
    hal_gclk_gen_config_t gclk0_cfg = {
        .gen = 0,
        .src = HAL_CLK_SRC_DPLL0,  /* 120 MHz from DPLL0 */
        .div = 1,                   /* Undivided */
        .idc = true,                /* Improve duty cycle */
    };
    hal_clock_configure_gclk_gen(&gclk0_cfg);
    s_gclk_freq[0] = 120000000U;
}

/**
 * Preset: HAL_CLOCK_PRESET_DPLL_120MHZ_XOSC32K
 *
 * 120 MHz CPU via DPLL0 with XOSC32K crystal reference — the production-
 * quality configuration matching the Miniscope-v4-Wire-Free firmware.
 *
 * Clock chain:
 *   XOSC32K (32.768 kHz) → GCLK2 → DFLL closed-loop (48 MHz)
 *     → GCLK3 ÷16 (3 MHz) → DPLL0 ×40 (120 MHz) → GCLK0 (120 MHz CPU)
 *
 * Benefits over the no-crystal preset:
 *   - Frequency accuracy: crystal is ±20 ppm vs. DFLL open-loop ±1-2%
 *   - Temperature stability: crystal compensates for thermal drift
 *   - Required for USB, precise UART baud rates, accurate timekeeping
 *
 * Full sequence:
 *   1.  Verify AUTOWS
 *   2.  Enable XOSC32K (crystal mode, wait for ready)
 *   3.  GCLK2 = XOSC32K ÷1 = 32.768 kHz (DFLL reference)
 *   4.  Reconfigure DFLL to closed-loop (MUL=1465, locked to GCLK2)
 *       - This temp-switches GCLK0 to OSCULP32K (~32 kHz)
 *       - CPU runs very slowly until step 8
 *   5.  GCLK3 = DFLL ÷16 = 3 MHz (DPLL0 reference)
 *   6.  Route GCLK3 → DPLL0 channel
 *   7.  Configure DPLL0: 3 MHz × 40 = 120 MHz, wait lock
 *   8.  Switch GCLK0 to DPLL0 (CPU jumps to 120 MHz)
 *   9.  Update frequency tracking
 *
 * Why DFLL → GCLK3 → DPLL, not XOSC32K → DPLL directly?
 *   The DPLL reference must be 32 kHz - 3.2 MHz. While 32.768 kHz is in
 *   range, using it directly requires LDR=3661 and LDRFRAC≈12, which is
 *   a very high multiply ratio. Locking is slower and more jitter-prone.
 *   The DFLL intermediate stage provides a higher, cleaner reference
 *   frequency (3 MHz) for faster, more stable DPLL lock. This is the
 *   standard production pattern used by both the Wire-Free and Adafruit
 *   SAMD51 boards.
 */
static void preset_dpll_120mhz_xosc32k(void)
{
    verify_autows();

    /* Step 1: Enable the external 32.768 kHz crystal oscillator.
     * This is the root precision reference for the entire clock chain.
     * CGM=1 (XT/standard) is correct for typical 32.768 kHz watch crystals.
     * Startup time 0x2 = 500ms — generous for reliable crystal startup. */
    hal_xosc32k_config_t xosc_cfg = {
        .startup  = 2,      /* ~500 ms startup (CYCLE16384) */
        .en32k    = true,   /* Enable 32.768 kHz output (used by GCLK2) */
        .en1k     = false,  /* 1.024 kHz output not needed */
        .ondemand = false,  /* Keep running continuously */
        .cgm      = 1,      /* XT = standard gain mode for watch crystals */
    };
    hal_clock_enable_xosc32k(&xosc_cfg);

    /* Step 2: GCLK2 = XOSC32K ÷1 = 32.768 kHz
     * This generator provides the reference clock for the DFLL's closed-
     * loop mode. Using GCLK2 follows the Atmel START convention. */
    hal_gclk_gen_config_t gclk2_cfg = {
        .gen = 2,
        .src = HAL_CLK_SRC_XOSC32K,    /* 32.768 kHz crystal */
        .div = 1,                       /* Undivided */
        .idc = false,
    };
    hal_clock_configure_gclk_gen(&gclk2_cfg);
    s_gclk_freq[2] = 32768U;

    /* Step 3: Reconfigure DFLL to closed-loop, locked to GCLK2.
     *
     * MUL = 1465: 32768 Hz × 1465 = 48,005,120 Hz ≈ 48 MHz
     *   (This is the closest integer multiple of 32.768 kHz to 48 MHz.
     *    The 0.01% error is negligible for all practical purposes.)
     *
     * CSTEP = 0x1f (31): max coarse step — allows fastest initial lock
     * FSTEP = 0x7f (127): max fine step — allows fastest fine tracking
     *   (These values match the Wire-Free reference configuration.)
     *
     * WARNING: After this call, GCLK0 is on OSCULP32K (~32 kHz).
     * The CPU is running very slowly until we switch GCLK0 to DPLL0. */
    hal_dfll_config_t dfll_cfg = {
        .mul      = 1465,
        .cstep    = 0x1f,
        .fstep    = 0x7f,
        .gclk_gen = 2,     /* GCLK2 = XOSC32K provides the reference */
    };
    hal_clock_configure_dfll_closed_loop(&dfll_cfg);

    /* Step 4: GCLK3 = DFLL ÷16 = 3 MHz (DPLL0 reference)
     * The DFLL is now locked at ~48 MHz in closed-loop mode.
     * We divide by 16 to get 3 MHz, which is within the DPLL's
     * reference input range of 32 kHz - 3.2 MHz. */
    hal_gclk_gen_config_t gclk3_cfg = {
        .gen = 3,
        .src = HAL_CLK_SRC_DFLL,   /* 48 MHz DFLL (now in closed-loop) */
        .div = 16,                  /* 48 MHz / 16 = 3 MHz */
        .idc = false,
    };
    hal_clock_configure_gclk_gen(&gclk3_cfg);
    s_gclk_freq[3] = DFLL_OUTPUT_HZ / 16U;  /* 3 MHz */

    /* Step 5: Route GCLK3 to DPLL0's reference input channel. */
    hal_clock_enable_gclk_channel(OSCCTRL_GCLK_ID_FDPLL0, 3);

    /* Step 6: Configure and enable DPLL0.
     * 3 MHz × (39 + 1) = 120 MHz exactly. */
    hal_dpll_config_t dpll0_cfg = {
        .ref_clk   = HAL_DPLL_REF_GCLK,
        .ref_hz    = 3000000U,
        .output_hz = 120000000U,
    };
    hal_clock_enable_dpll(0, &dpll0_cfg);

    /* Step 7: Switch GCLK0 to DPLL0.
     * This is the critical moment — the CPU jumps from ~32 kHz (OSCULP32K,
     * where it's been since the DFLL reconfiguration) to 120 MHz.
     * IDC=1 improves duty cycle of the output clock. */
    hal_gclk_gen_config_t gclk0_cfg = {
        .gen = 0,
        .src = HAL_CLK_SRC_DPLL0,  /* 120 MHz from DPLL0 */
        .div = 1,                   /* Undivided */
        .idc = true,
    };
    hal_clock_configure_gclk_gen(&gclk0_cfg);
    s_gclk_freq[0] = 120000000U;
}

/**
 * Preset: HAL_CLOCK_PRESET_DPLL_60MHZ_XOSC32K
 *
 * 60 MHz CPU via DPLL0 with XOSC32K crystal reference.
 * Identical to the 120 MHz XOSC32K preset except GCLK0 divides
 * DPLL0 output by 2 to produce 60 MHz CPU clock.
 *
 * Clock chain:
 *   XOSC32K (32.768 kHz) → GCLK2 → DFLL closed-loop (48 MHz)
 *     → GCLK3 ÷16 (3 MHz) → DPLL0 ×40 (120 MHz) → GCLK0 ÷2 (60 MHz)
 *
 * This matches the original Wire-Free V4 firmware's clock tree
 * (DPLL0=120 MHz, GCLK0 div=2 → 60 MHz CPU).
 */
static void preset_dpll_60mhz_xosc32k(void)
{
    verify_autows();

    /* Steps 1-6: identical to 120 MHz XOSC32K preset */

    /* Step 1: Enable XOSC32K */
    hal_xosc32k_config_t xosc_cfg = {
        .startup  = 2,
        .en32k    = true,
        .en1k     = false,
        .ondemand = false,
        .cgm      = 1,
    };
    hal_clock_enable_xosc32k(&xosc_cfg);

    /* Step 2: GCLK2 = XOSC32K ÷1 = 32.768 kHz */
    hal_gclk_gen_config_t gclk2_cfg = {
        .gen = 2,
        .src = HAL_CLK_SRC_XOSC32K,
        .div = 1,
        .idc = false,
    };
    hal_clock_configure_gclk_gen(&gclk2_cfg);
    s_gclk_freq[2] = 32768U;

    /* Step 3: DFLL closed-loop locked to GCLK2 */
    hal_dfll_config_t dfll_cfg = {
        .mul      = 1465,
        .cstep    = 0x1f,
        .fstep    = 0x7f,
        .gclk_gen = 2,
    };
    hal_clock_configure_dfll_closed_loop(&dfll_cfg);

    /* Step 4: GCLK3 = DFLL ÷16 = 3 MHz */
    hal_gclk_gen_config_t gclk3_cfg = {
        .gen = 3,
        .src = HAL_CLK_SRC_DFLL,
        .div = 16,
        .idc = false,
    };
    hal_clock_configure_gclk_gen(&gclk3_cfg);
    s_gclk_freq[3] = DFLL_OUTPUT_HZ / 16U;

    /* Step 5: Route GCLK3 to DPLL0 */
    hal_clock_enable_gclk_channel(OSCCTRL_GCLK_ID_FDPLL0, 3);

    /* Step 6: DPLL0 = 3 MHz × 40 = 120 MHz */
    hal_dpll_config_t dpll0_cfg = {
        .ref_clk   = HAL_DPLL_REF_GCLK,
        .ref_hz    = 3000000U,
        .output_hz = 120000000U,
    };
    hal_clock_enable_dpll(0, &dpll0_cfg);

    /* Step 7: GCLK0 = DPLL0 ÷2 = 60 MHz (the only difference) */
    hal_gclk_gen_config_t gclk0_cfg = {
        .gen = 0,
        .src = HAL_CLK_SRC_DPLL0,
        .div = 2,
        .idc = true,
    };
    hal_clock_configure_gclk_gen(&gclk0_cfg);
    s_gclk_freq[0] = 60000000U;
}

/* ======================================================================
 * Preset Dispatcher
 * ====================================================================== */

void hal_clock_init_preset(hal_clock_preset_t preset)
{
    switch (preset) {
    case HAL_CLOCK_PRESET_DFLL_48MHZ:
        preset_dfll_48mhz();
        break;

    case HAL_CLOCK_PRESET_DPLL_120MHZ:
        preset_dpll_120mhz();
        break;

    case HAL_CLOCK_PRESET_DPLL_120MHZ_XOSC32K:
        preset_dpll_120mhz_xosc32k();
        break;

    case HAL_CLOCK_PRESET_DPLL_60MHZ_XOSC32K:
        preset_dpll_60mhz_xosc32k();
        break;

    default:
        __BKPT(0);  /* unknown preset */
        break;
    }
}
