# Cross-compilation toolchain file for ARM Cortex-M (arm-none-eabi-gcc)
#
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake ..

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Toolchain prefix - override via environment variable if needed
if(NOT DEFINED TOOLCHAIN_PREFIX)
    set(TOOLCHAIN_PREFIX arm-none-eabi)
endif()

# Find toolchain programs
find_program(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
find_program(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
find_program(CMAKE_AR           ${TOOLCHAIN_PREFIX}-ar)
find_program(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}-objcopy)
find_program(CMAKE_OBJDUMP      ${TOOLCHAIN_PREFIX}-objdump)
find_program(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}-size)
find_program(CMAKE_GDB          ${TOOLCHAIN_PREFIX}-gdb)

# Use pipes instead of temp files between compilation stages.
# Fixes "Cannot create temporary file in C:\Windows\: Permission denied" on
# MSYS2 where Ninja's subprocess environment may lack TMP/TEMP variables.
set(CMAKE_C_FLAGS_INIT "-pipe")
set(CMAKE_ASM_FLAGS_INIT "-pipe")

# Don't try to run test executables on the host during configure
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Search paths: don't look in host system directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
