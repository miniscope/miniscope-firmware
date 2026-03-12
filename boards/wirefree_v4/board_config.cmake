# =============================================================================
# Wire-Free V4 Board Configuration
# =============================================================================
# Miniscope Wire-Free V4 — SAMD51J20A (64-pin, 1MB flash, 256KB RAM)
# Crystal: 32.768 kHz XOSC32K for production-quality clock chain
# =============================================================================

# MCU variant (lowercase, matches DFP startup/system filenames)
set(BOARD_MCU "samd51j20a")

# Preprocessor define for the device header
set(BOARD_DEVICE_DEFINE "__SAMD51J20A__")

# Linker script — use DFP-provided flash script
set(BOARD_LINKER_SCRIPT "${DFP_DIR}/samd51a/gcc/gcc/samd51j20a_flash.ld")

# Firmware project
set(BOARD_FIRMWARE_PROJECT "fw_miniscope")

# Board-specific compile definitions
set(BOARD_DEFINITIONS
    F_CPU=120000000UL
    HAL_CLOCK_PRESET=HAL_CLOCK_PRESET_DPLL_120MHZ_XOSC32K

    # Sensor selection
    FW_SENSOR_PYTHON480

    # Output mode
    FW_OUTPUT_SD

    # Feature flags
    FW_ENABLE_LED_PWM
    FW_ENABLE_EWL
    FW_ENABLE_BATTERY_MONITOR
    FW_ENABLE_IR_RECEIVER
    FW_ENABLE_DMA
)

# 16KB stack — DMA buffers consume most RAM
set(BOARD_STACK_SIZE 0x4000)
