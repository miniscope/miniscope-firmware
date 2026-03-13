# TinyUSB INTERFACE library — minimal CDC device stack
#
# Provides the TinyUSB core + CDC class + SAMD port as an INTERFACE library.
# Boards opt in by setting FW_ENABLE_USB in their BOARD_DEFINITIONS, which
# causes add_board_firmware() to link this target.
#
# The tusb_config.h expected by TinyUSB is provided by samd51_common at:
#   firmware/samd51/common/include/tusb_config.h

set(TINYUSB_DIR "${CMAKE_SOURCE_DIR}/third_party/tinyusb")

add_library(tinyusb INTERFACE)

target_sources(tinyusb INTERFACE
    # Core
    ${TINYUSB_DIR}/src/tusb.c
    ${TINYUSB_DIR}/src/common/tusb_fifo.c
    # Device stack
    ${TINYUSB_DIR}/src/device/usbd.c
    ${TINYUSB_DIR}/src/device/usbd_control.c
    # CDC class
    ${TINYUSB_DIR}/src/class/cdc/cdc_device.c
    # SAMD port
    ${TINYUSB_DIR}/src/portable/microchip/samd/dcd_samd.c
)

target_include_directories(tinyusb INTERFACE
    # DFP v3 compat wrapper must come before the DFP include so that
    # TinyUSB's #include "sam.h" finds our wrapper first.
    ${CMAKE_SOURCE_DIR}/cmake/tinyusb_compat
    ${TINYUSB_DIR}/src
)

# Suppress warnings in third-party TinyUSB code.
# The INTERFACE_SYSTEM flag would be ideal but doesn't exist for INTERFACE
# libraries, so we add specific suppressions.
target_compile_options(tinyusb INTERFACE
    -Wno-cast-align
)
