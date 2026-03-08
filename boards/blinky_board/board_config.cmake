# =============================================================================
# TEMPLATE: Board Configuration
# =============================================================================
# Copy this entire boards/blinky_board/ directory for a new board, or run:
#   bash scripts/new_board.sh <board_name> [mcu_variant]
#
# After copying, update the variables below, then:
#   1. Add  add_board_firmware(<board_name>)  in the root CMakeLists.txt
#   2. Add configure + build presets in CMakePresets.json
#   3. CI will auto-discover the new preset — no workflow changes needed
# =============================================================================

# MCU variant (lowercase, matches DFP startup/system filenames)
# Examples: samd51p20a, samd51j20a, samd51g19a
set(BOARD_MCU "samd51p20a")

# Preprocessor define for the device header (matches DFP convention)
# Must match a header in third_party/samd51_dfp/samd51a/include/
set(BOARD_DEVICE_DEFINE "__SAMD51P20A__")

# Linker script - use the DFP-provided flash linker script directly.
# Override with a board-local linker.ld if you need custom memory layout.
set(BOARD_LINKER_SCRIPT "${DFP_DIR}/samd51a/gcc/gcc/samd51p20a_flash.ld")

# Which firmware project this board builds (CMake library target name)
set(BOARD_FIRMWARE_PROJECT "fw_blinky")

# ---------------------------------------------------------------------------
# Optional variables
# ---------------------------------------------------------------------------

# BOARD_DEFINITIONS - Extra compile definitions (feature flags, pin overrides)
# Example:
#   set(BOARD_DEFINITIONS
#       FW_ENABLE_UART
#       BLINKY_LED_PIN=16
#       BLINKY_LED_PORT=0
#   )

# BOARD_SOURCES - Extra source files relative to the board directory
# Example:
#   set(BOARD_SOURCES
#       drivers/sensor.c
#   )
