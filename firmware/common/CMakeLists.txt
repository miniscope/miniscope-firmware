# Cross-MCU common library
#
# Platform-independent utilities, data structures, and algorithms that
# can be shared across any MCU family. Code here must NOT depend on
# any specific MCU headers or peripherals.
#
# Examples of what belongs here:
#   - CRC/checksum routines
#   - Ring buffers
#   - Protocol parsers (e.g. serial command protocol)
#   - Math utilities
#
# MCU-specific common code (e.g. SAMD51 bit-bang I2C) belongs in
# firmware/<mcu>/common/ instead.

add_library(fw_platform_common INTERFACE)

target_include_directories(fw_platform_common INTERFACE
    include
)
