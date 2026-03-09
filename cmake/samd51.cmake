# SAMD51-family common build settings
#
# Included by the top-level CMakeLists.txt after project() is called.
# Provides:
#   - Cortex-M4F compile/link flags
#   - add_board_firmware() function for per-board targets
#   - Paths to vendored DFP and CMSIS headers

# ---------------------------------------------------------------------------
# Paths to vendored third-party content
# ---------------------------------------------------------------------------
set(CMSIS_CORE_INCLUDE "${CMAKE_SOURCE_DIR}/third_party/CMSIS_5/CMSIS/Core/Include")
set(DFP_DIR            "${CMAKE_SOURCE_DIR}/third_party/samd51_dfp")
set(DFP_INCLUDE        "${DFP_DIR}/samd51a/include")

# Validate that DFP content exists
if(NOT EXISTS "${DFP_INCLUDE}")
    message(FATAL_ERROR
        "SAMD51 DFP headers not found at ${DFP_INCLUDE}\n"
        "Run: scripts/fetch_deps.sh   (or see README)")
endif()
if(NOT EXISTS "${CMSIS_CORE_INCLUDE}/core_cm4.h")
    message(FATAL_ERROR
        "CMSIS Core headers not found at ${CMSIS_CORE_INCLUDE}\n"
        "Run: git submodule update --init   (or see README)")
endif()

# ---------------------------------------------------------------------------
# Cortex-M4F flags (SAMD51 has single-precision FPU)
# ---------------------------------------------------------------------------
set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
)

set(COMMON_C_FLAGS
    ${MCU_FLAGS}
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wall
    -Wextra
    -Wshadow
    -Wno-unused-parameter
)

set(COMMON_LINK_FLAGS
    ${MCU_FLAGS}
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    --specs=nosys.specs
    --specs=nano.specs
)

# ---------------------------------------------------------------------------
# add_board_firmware(<board_name>)
#
# Creates an executable target fw_<board_name>.elf and post-build steps to
# produce .hex, .bin, and .map outputs.
#
# All firmware project sources are compiled as part of the executable target
# (not as a separate library). This ensures board-specific defines like the
# MCU device define (__SAMD51P20A__) and feature flags are visible to all
# source files at compile time.
#
# Each board's board_config.cmake must set:
#   BOARD_MCU              - MCU variant lowercase (e.g. "samd51p20a")
#   BOARD_DEVICE_DEFINE    - Preprocessor define (e.g. "__SAMD51P20A__")
#   BOARD_LINKER_SCRIPT    - Linker script path (relative to board dir or absolute)
#   BOARD_FIRMWARE_PROJECT - INTERFACE library target (e.g. "fw_blinky")
#
# Optional:
#   BOARD_DEFINITIONS      - Extra compile definitions
#   BOARD_SOURCES          - Extra source files (relative to board dir)
# ---------------------------------------------------------------------------
function(add_board_firmware board_name)
    set(BOARD_DIR "${CMAKE_SOURCE_DIR}/boards/${board_name}")

    # Load board-specific configuration
    if(NOT EXISTS "${BOARD_DIR}/board_config.cmake")
        message(FATAL_ERROR "Missing ${BOARD_DIR}/board_config.cmake")
    endif()
    include("${BOARD_DIR}/board_config.cmake")

    # Validate required variables
    foreach(var BOARD_MCU BOARD_DEVICE_DEFINE BOARD_LINKER_SCRIPT BOARD_FIRMWARE_PROJECT)
        if(NOT DEFINED ${var})
            message(FATAL_ERROR "${var} not set in ${BOARD_DIR}/board_config.cmake")
        endif()
    endforeach()

    # Resolve linker script path
    if(NOT IS_ABSOLUTE "${BOARD_LINKER_SCRIPT}")
        set(BOARD_LINKER_SCRIPT "${BOARD_DIR}/${BOARD_LINKER_SCRIPT}")
    endif()

    # Find DFP startup file for this MCU
    string(TOLOWER "${BOARD_MCU}" mcu_lower)
    set(STARTUP_FILE "${DFP_DIR}/samd51a/gcc/gcc/startup_${mcu_lower}.c")
    set(SYSTEM_FILE  "${DFP_DIR}/samd51a/gcc/system_${mcu_lower}.c")

    if(NOT EXISTS "${STARTUP_FILE}")
        message(FATAL_ERROR "Startup file not found: ${STARTUP_FILE}")
    endif()
    if(NOT EXISTS "${SYSTEM_FILE}")
        message(FATAL_ERROR "System file not found: ${SYSTEM_FILE}")
    endif()

    # Collect board-local sources (board.c is auto-detected; main.c is optional
    # here because the firmware project may provide it via INTERFACE_FW_SOURCES)
    set(BOARD_EXTRA_SOURCES "")
    if(EXISTS "${BOARD_DIR}/main.c")
        list(APPEND BOARD_EXTRA_SOURCES "${BOARD_DIR}/main.c")
    endif()
    if(EXISTS "${BOARD_DIR}/board.c")
        list(APPEND BOARD_EXTRA_SOURCES "${BOARD_DIR}/board.c")
    endif()
    if(DEFINED BOARD_SOURCES)
        foreach(src ${BOARD_SOURCES})
            if(NOT IS_ABSOLUTE "${src}")
                list(APPEND BOARD_EXTRA_SOURCES "${BOARD_DIR}/${src}")
            else()
                list(APPEND BOARD_EXTRA_SOURCES "${src}")
            endif()
        endforeach()
    endif()

    # Get firmware project sources from the INTERFACE target's custom property
    get_target_property(FW_PROJECT_SOURCES ${BOARD_FIRMWARE_PROJECT} INTERFACE_FW_SOURCES)
    if(NOT FW_PROJECT_SOURCES)
        set(FW_PROJECT_SOURCES "")
    endif()

    # Create firmware executable - all sources compiled together so defines
    # (device, board, features) are consistent across all translation units.
    add_executable(fw_${board_name}.elf
        ${STARTUP_FILE}
        ${SYSTEM_FILE}
        ${FW_PROJECT_SOURCES}
        ${BOARD_EXTRA_SOURCES}
    )

    # Link against the firmware project INTERFACE library (for include paths
    # and transitive dependencies like samd51_common and fw_platform_common)
    target_link_libraries(fw_${board_name}.elf PRIVATE ${BOARD_FIRMWARE_PROJECT})

    # Include directories (board-specific headers)
    target_include_directories(fw_${board_name}.elf PRIVATE
        ${BOARD_DIR}
        ${CMAKE_SOURCE_DIR}/boards/board_common
    )

    # Compile definitions (device + board ID + feature flags)
    target_compile_definitions(fw_${board_name}.elf PRIVATE
        ${BOARD_DEVICE_DEFINE}
        FW_BOARD_${board_name}=1
    )
    if(DEFINED BOARD_DEFINITIONS)
        target_compile_definitions(fw_${board_name}.elf PRIVATE ${BOARD_DEFINITIONS})
    endif()

    # Compile options
    target_compile_options(fw_${board_name}.elf PRIVATE ${COMMON_C_FLAGS})

    # Linker options
    target_link_options(fw_${board_name}.elf PRIVATE
        ${COMMON_LINK_FLAGS}
        -T${BOARD_LINKER_SCRIPT}
        -Wl,-Map=$<TARGET_FILE_DIR:fw_${board_name}.elf>/fw_${board_name}.map
    )

    # Post-build: generate .hex, .bin, print size
    add_custom_command(TARGET fw_${board_name}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:fw_${board_name}.elf> fw_${board_name}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:fw_${board_name}.elf> fw_${board_name}.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:fw_${board_name}.elf>
        WORKING_DIRECTORY $<TARGET_FILE_DIR:fw_${board_name}.elf>
        COMMENT "Generating HEX/BIN for ${board_name}"
    )
endfunction()
