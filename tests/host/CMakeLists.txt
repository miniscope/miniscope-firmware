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
target_link_libraries(test_app_blinky PRIVATE unity mock_hal)
add_test(NAME test_app_blinky COMMAND test_app_blinky)
