# Git-tag based firmware versioning
#
# Extracts version info from `git describe --tags`. If no tags exist,
# falls back to 0.0.0 with just the short hash.
#
# Sets the following variables in parent scope:
#   FW_VERSION_MAJOR   - Major version number
#   FW_VERSION_MINOR   - Minor version number
#   FW_VERSION_PATCH   - Patch version number
#   FW_VERSION_STRING  - Full version string (e.g. "1.2.3-5-gabcdef-dirty")
#   FW_VERSION_GIT_HASH - Short git commit hash

execute_process(
    COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE GIT_RESULT
)

if(GIT_RESULT EQUAL 0 AND GIT_DESCRIBE)
    set(FW_VERSION_STRING "${GIT_DESCRIBE}")
else()
    set(FW_VERSION_STRING "0.0.0-unknown")
endif()

# Extract short hash
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE FW_VERSION_GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(NOT FW_VERSION_GIT_HASH)
    set(FW_VERSION_GIT_HASH "unknown")
endif()

# Parse vMAJOR.MINOR.PATCH from the version string
if(FW_VERSION_STRING MATCHES "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    set(FW_VERSION_MAJOR "${CMAKE_MATCH_1}")
    set(FW_VERSION_MINOR "${CMAKE_MATCH_2}")
    set(FW_VERSION_PATCH "${CMAKE_MATCH_3}")
else()
    set(FW_VERSION_MAJOR 0)
    set(FW_VERSION_MINOR 0)
    set(FW_VERSION_PATCH 0)
endif()

message(STATUS "Firmware version: ${FW_VERSION_STRING} (${FW_VERSION_MAJOR}.${FW_VERSION_MINOR}.${FW_VERSION_PATCH}, hash: ${FW_VERSION_GIT_HASH})")
