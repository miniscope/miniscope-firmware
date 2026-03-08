# Board configuration for blinky_board
#
# Minimal demo board targeting the ATSAMD51P20A.
# Copy this file as a starting point for new board definitions.

# MCU variant (lowercase, matches DFP startup/system filenames)
set(BOARD_MCU "samd51p20a")

# Preprocessor define for the device header (matches DFP convention)
set(BOARD_DEVICE_DEFINE "__SAMD51P20A__")

# Linker script - use the DFP-provided flash linker script directly.
# Override with a board-local linker.ld if you need custom memory layout.
set(BOARD_LINKER_SCRIPT "${DFP_DIR}/samd51a/gcc/gcc/samd51p20a_flash.ld")

# Which firmware project this board builds (CMake library target name)
set(BOARD_FIRMWARE_PROJECT "fw_blinky")

# Board-specific compile definitions (feature flags)
# LED pin configuration is in firmware/samd51/blinky/include/blinky_config.h
# Override here if your board uses different pins:
# set(BOARD_DEFINITIONS
#     BLINKY_LED_PIN=16
#     BLINKY_LED_PORT=0
# )
