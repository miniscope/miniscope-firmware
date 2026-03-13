# Host-based unit tests using Unity test framework
#
# These tests compile firmware application code against mock HAL headers
# on the host machine (no cross-compilation). Mock headers in tests/host/mocks/
# shadow real HAL headers by appearing first on the include path.

set(UNITY_DIR "${CMAKE_SOURCE_DIR}/third_party/Unity")

if(NOT EXISTS "${UNITY_DIR}/src/unity.c")
    message(FATAL_ERROR
        "Unity not found at ${UNITY_DIR}\n"
        "Run: bash scripts/fetch_test_deps.sh")
endif()

# Unity as a static library
add_library(unity STATIC
    ${UNITY_DIR}/src/unity.c
)
target_include_directories(unity PUBLIC
    ${UNITY_DIR}/src
)

# Mock HAL implementations
add_library(mock_hal STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/mock_hal.c
)
target_include_directories(mock_hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

# ---------------------------------------------------------------------------
# Test: app_blinky
# ---------------------------------------------------------------------------
add_executable(test_app_blinky
    ${CMAKE_CURRENT_SOURCE_DIR}/test_app_blinky.c
    ${CMAKE_SOURCE_DIR}/firmware/samd51/blinky/src/app_blinky.c
)
target_include_directories(test_app_blinky PRIVATE
    # Mocks first — shadows real HAL headers
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    # Application headers
    ${CMAKE_SOURCE_DIR}/firmware/samd51/blinky/include
    ${CMAKE_SOURCE_DIR}/firmware/common/include
    ${CMAKE_BINARY_DIR}/generated
)
target_compile_definitions(test_app_blinky PRIVATE FW_ENABLE_USB)
target_link_libraries(test_app_blinky PRIVATE unity mock_hal)
add_test(NAME test_app_blinky COMMAND test_app_blinky)

# ---------------------------------------------------------------------------
# Test: sd_format (pack/unpack round-trip)
# ---------------------------------------------------------------------------
add_executable(test_sd_format
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sd_format.c
    ${CMAKE_SOURCE_DIR}/firmware/samd51/miniscope/src/sd_format.c
)
target_include_directories(test_sd_format PRIVATE
    ${CMAKE_SOURCE_DIR}/firmware/samd51/miniscope/include
    ${CMAKE_SOURCE_DIR}/firmware/common/include
)
target_link_libraries(test_sd_format PRIVATE unity)
add_test(NAME test_sd_format COMMAND test_sd_format)

# ---------------------------------------------------------------------------
# Test: miniscope_buffer (producer-consumer ring)
# ---------------------------------------------------------------------------
add_executable(test_miniscope_buffer
    ${CMAKE_CURRENT_SOURCE_DIR}/test_miniscope_buffer.c
    ${CMAKE_SOURCE_DIR}/firmware/samd51/miniscope/src/miniscope_buffer.c
)
target_include_directories(test_miniscope_buffer PRIVATE
    ${CMAKE_SOURCE_DIR}/firmware/samd51/miniscope/include
    ${CMAKE_SOURCE_DIR}/firmware/common/include
)
target_link_libraries(test_miniscope_buffer PRIVATE unity)
add_test(NAME test_miniscope_buffer COMMAND test_miniscope_buffer)

# ---------------------------------------------------------------------------
# Test: miniscope_state (recording state machine)
# ---------------------------------------------------------------------------
add_executable(test_miniscope_state
    ${CMAKE_CURRENT_SOURCE_DIR}/test_miniscope_state.c
    ${CMAKE_SOURCE_DIR}/firmware/samd51/miniscope/src/miniscope_state.c
)
target_include_directories(test_miniscope_state PRIVATE
    ${CMAKE_SOURCE_DIR}/firmware/samd51/miniscope/include
    ${CMAKE_SOURCE_DIR}/firmware/common/include
)
target_link_libraries(test_miniscope_state PRIVATE unity)
add_test(NAME test_miniscope_state COMMAND test_miniscope_state)

# ---------------------------------------------------------------------------
# Test: encode_8b10b (line encoding)
# ---------------------------------------------------------------------------
add_executable(test_encode_8b10b
    ${CMAKE_CURRENT_SOURCE_DIR}/test_encode_8b10b.c
    ${CMAKE_SOURCE_DIR}/firmware/common/src/encode_8b10b.c
)
target_include_directories(test_encode_8b10b PRIVATE
    ${CMAKE_SOURCE_DIR}/firmware/common/include
)
target_link_libraries(test_encode_8b10b PRIVATE unity)
add_test(NAME test_encode_8b10b COMMAND test_encode_8b10b)
