# =============================================================================
# Board Configuration: Adafruit Grand Central M4
# =============================================================================
# Adafruit Grand Central M4 Express (ATSAMD51P20A)
# Built-in LED on PB1 (active high)
# =============================================================================

# MCU variant (lowercase, matches DFP startup/system filenames)
set(BOARD_MCU "samd51p20a")

# Preprocessor define for the device header (matches DFP convention)
set(BOARD_DEVICE_DEFINE "__SAMD51P20A__")

# Linker script - use the DFP-provided flash linker script
set(BOARD_LINKER_SCRIPT "${DFP_DIR}/samd51a/gcc/gcc/samd51p20a_flash.ld")

# Which firmware project this board builds (CMake library target name)
set(BOARD_FIRMWARE_PROJECT "fw_blinky")

# ---------------------------------------------------------------------------
# Board definitions — clock preset + LED pin overrides for PB1
# ---------------------------------------------------------------------------
set(BOARD_DEFINITIONS
    F_CPU=48000000UL
    HAL_CLOCK_PRESET=HAL_CLOCK_PRESET_DFLL_48MHZ
    BLINKY_LED_PORT=1
    BLINKY_LED_PIN=1
    FW_ENABLE_USB
)
