cmake_minimum_required(VERSION 3.21)
project(miniscope-firmware C ASM)

# ---------------------------------------------------------------------------
# Host-based testing (native compiler, no cross-compilation)
# ---------------------------------------------------------------------------
if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    enable_testing()
    add_subdirectory(tests)
    return()  # Skip cross-compiled targets when building tests
endif()

# SAMD51 common build settings and add_board_firmware() function
include(cmake/samd51.cmake)

# ---------------------------------------------------------------------------
# Firmware libraries (layered)
#
# The dependency chain is:
#
#   fw_platform_common       Cross-MCU utilities (no HW deps)
#        |
#   samd51_common            SAMD51-family shared code (HAL wrappers, bit-bang I2C, etc.)
#        |
#   fw_<project>             Application firmware (blinky, miniscope_v4, etc.)
#        |
#   fw_<board>.elf           Final linked binary (startup + board.c + linker script)
#
# Structure:
#   firmware/
#     common/            - Cross-MCU platform utilities
#     samd51/
#       common/          - SAMD51-specific shared code
#       blinky/          - LED blink demo project
#       <your_project>/  - Your application firmware
# ---------------------------------------------------------------------------
add_subdirectory(firmware/common)
add_subdirectory(firmware/samd51/common)
add_subdirectory(firmware/samd51/blinky)
add_subdirectory(firmware/samd51/miniscope)

# Optional Doxygen documentation target
include(cmake/docs.cmake)

# ---------------------------------------------------------------------------
# Board targets
#
# Each call creates fw_<board_name>.elf + .hex + .bin + .map
# To add a new board:
#   1. Create boards/<name>/board_config.cmake
#   2. Create boards/<name>/board.h (and optionally board.c)
#   3. Add add_board_firmware(<name>) below
#   4. Add a CMakePresets.json entry if desired
# ---------------------------------------------------------------------------
add_board_firmware(blinky_board)
add_board_firmware(grand_central_m4)
add_board_firmware(wirefree_v4)
