# Miniscope Firmware - Development Guide

## Project Overview

Multi-board, multi-MCU firmware framework for Miniscope projects. Migrated from Microchip Studio to GCC + CMake + VS Code + GitHub Actions.

## Architecture

```
firmware/
  common/              Cross-MCU platform utilities (no HW deps)
  samd51/
    common/            SAMD51-family shared code (HAL wrappers, drivers)
    blinky/            Demo blinky project
    <project>/         Each project is self-contained
boards/
  board_common/        Shared board interface (board_common.h)
  <board_name>/        Per-board config, pin defs, linker script
```

Dependency chain: `fw_platform_common → samd51_common → fw_<project> → fw_<board>.elf`

## Build

```bash
# First-time setup
git submodule update --init
bash scripts/fetch_deps.sh

# Build a board
cmake --preset blinky_board
cmake --build --preset blinky_board
```

Requires: `arm-none-eabi-gcc`, `cmake` (≥3.21), `ninja`

## Adding a New Board

1. Create `boards/<name>/board_config.cmake` (copy from blinky_board)
2. Create `boards/<name>/board.h` and `board.c`
3. Create or copy `boards/<name>/linker.ld`
4. Add `add_board_firmware(<name>)` in root `CMakeLists.txt`
5. Add preset in `CMakePresets.json`

## Adding a New Firmware Project

1. Create `firmware/<mcu>/<project>/` with `CMakeLists.txt`, `src/`, `include/`
2. Define a library target (e.g., `fw_<project>`) linking against `samd51_common`
3. Add `add_subdirectory()` in root `CMakeLists.txt`
4. Set `BOARD_FIRMWARE_PROJECT` in board configs that should use it

## Debug (VS Code)

Uses Cortex-Debug extension with OpenOCD + Atmel-ICE over CMSIS-DAP/SWD.
Config: `.vscode/launch.json`

## Flash

```bash
bash scripts/flash_openocd.sh build/blinky_board/fw_blinky_board.elf
```

## CI

GitHub Actions matrix builds all boards on push/PR. Tag pushes (`v*`) create releases.

## Conventions

- Board-specific code in `boards/<name>/`, never in firmware project dirs
- Feature flags via `target_compile_definitions` in `board_config.cmake`, named `FW_ENABLE_<FEATURE>`
- MCU-specific shared code in `firmware/<mcu>/common/`
- Platform-independent code in `firmware/common/`
- All boards must implement `board_init()` from `board_common.h`
