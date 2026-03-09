# Blinky firmware project
#
# Minimal LED-blinking firmware for testing the build/debug/flash toolchain.
#
# This is an INTERFACE library: it does not compile anything itself. It provides
# source file paths and include directories that are compiled as part of each
# board's executable target. This is necessary because the device define
# (e.g. __SAMD51P20A__) varies per board and must be set at compile time.

add_library(fw_blinky INTERFACE)

# Sources are exposed via a custom property so add_board_firmware() can
# compile them as part of the executable target.
set_target_properties(fw_blinky PROPERTIES
    INTERFACE_FW_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c;${CMAKE_CURRENT_SOURCE_DIR}/src/app_blinky.c"
)

target_include_directories(fw_blinky INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(fw_blinky INTERFACE samd51_common)
