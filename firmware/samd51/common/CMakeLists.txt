# SAMD51 common library
#
# Shared code for all SAMD51 firmware projects. This is where MCU-family-level
# reusable code lives: HAL wrappers, bit-bang I2C, SPI helpers, GPIO utilities,
# timer abstractions, etc.
#
# Individual firmware projects (blinky, miniscope_v4, ...) link against this.
# Code here CAN depend on SAMD51 device headers but must NOT depend on any
# specific board or project.

add_library(samd51_common INTERFACE)

target_include_directories(samd51_common INTERFACE
    include
    ${CMSIS_CORE_INCLUDE}
    ${DFP_INCLUDE}
)

target_sources(samd51_common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hal_systick.c
)

target_compile_options(samd51_common INTERFACE ${COMMON_C_FLAGS})

# Inherit cross-MCU common utilities
target_link_libraries(samd51_common INTERFACE fw_platform_common)
