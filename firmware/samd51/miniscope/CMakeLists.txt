# Miniscope firmware project
#
# Core capture-to-storage firmware for Miniscope devices. Supports multiple
# sensor types and output modes via compile-time feature flags set in each
# board's board_config.cmake.
#
# This is an INTERFACE library: sources are compiled as part of each board's
# executable target so device defines are visible to all translation units.

add_library(fw_miniscope INTERFACE)

# Sources exposed via custom property for add_board_firmware()
set_target_properties(fw_miniscope PROPERTIES
    INTERFACE_FW_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c;\
${CMAKE_CURRENT_SOURCE_DIR}/src/app_miniscope.c;\
${CMAKE_CURRENT_SOURCE_DIR}/src/miniscope_state.c;\
${CMAKE_CURRENT_SOURCE_DIR}/src/miniscope_buffer.c;\
${CMAKE_CURRENT_SOURCE_DIR}/src/pcc_capture.c;\
${CMAKE_CURRENT_SOURCE_DIR}/src/sd_raw.c;\
${CMAKE_CURRENT_SOURCE_DIR}/src/sd_format.c;\
${CMAKE_CURRENT_SOURCE_DIR}/src/sensor_python480.c;\
${CMAKE_CURRENT_SOURCE_DIR}/src/led_control.c;\
${CMAKE_CURRENT_SOURCE_DIR}/src/ewl_driver.c;\
${CMAKE_CURRENT_SOURCE_DIR}/src/battery_monitor.c;\
${CMAKE_CURRENT_SOURCE_DIR}/src/ir_receiver.c;\
${CMAKE_CURRENT_SOURCE_DIR}/src/debug_output.c"
)

target_include_directories(fw_miniscope INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(fw_miniscope INTERFACE samd51_common)
