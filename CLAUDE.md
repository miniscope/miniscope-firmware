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

Quick start:
```bash
bash scripts/new_board.sh <name> [mcu_variant]
```

Manual steps (or after running the script):

1. Create `boards/<name>/` (copy from `boards/blinky_board/` or use the script)
2. Edit `boards/<name>/board_config.cmake` — set MCU, firmware project, feature flags
3. Edit `boards/<name>/board.h` and `board.c` for your hardware
4. Add `add_board_firmware(<name>)` in root `CMakeLists.txt`
5. Add configure + build presets in `CMakePresets.json`

CI auto-discovers boards from non-hidden, non-debug configure presets — no workflow changes needed.

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

## USB CDC

TinyUSB (git submodule at `third_party/tinyusb`, tag 0.17.0) provides the USB device stack. Boards opt in via `FW_ENABLE_USB` in `BOARD_DEFINITIONS`. The `cmake/tinyusb.cmake` INTERFACE library is conditionally linked by `add_board_firmware()`. A DFP v3 compatibility shim (`tusb_dfp_v3_compat.h`) bridges the register naming gap between our DFP v3.8.x and TinyUSB's v2-style access patterns. Use `--recurse-submodules` on clone or `git submodule update --init` to fetch TinyUSB.

## Conventions

- Board-specific code in `boards/<name>/`, never in firmware project dirs
- Feature flags via `target_compile_definitions` in `board_config.cmake`, named `FW_ENABLE_<FEATURE>`
- MCU-specific shared code in `firmware/<mcu>/common/`
- Platform-independent code in `firmware/common/`
- All boards must implement `board_init()` from `board_common.h`
