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

include(${CMAKE_SOURCE_DIR}/cmake/version.cmake)

add_library(fw_platform_common INTERFACE)

target_include_directories(fw_platform_common INTERFACE
    include
    ${CMAKE_BINARY_DIR}/generated
)

# Generate fw_version.h from git tag info
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/fw_version.h.in
    ${CMAKE_BINARY_DIR}/generated/fw_version.h
    @ONLY
)

# Version source compiled in every board executable via INTERFACE
target_sources(fw_platform_common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fw_version.c
)
