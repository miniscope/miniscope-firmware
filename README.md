# Miniscope Firmware

[![CI](https://github.com/miniscope/miniscope-firmware/actions/workflows/ci.yml/badge.svg)](https://github.com/miniscope/miniscope-firmware/actions/workflows/ci.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)

Open-source firmware framework for [Miniscope](https://miniscope.org) hardware, built with GCC + CMake.

## Overview

This repository provides a multi-board, multi-MCU firmware framework for Miniscope projects. It replaces the legacy Microchip Studio build system with a portable GCC + CMake + Ninja toolchain, enabling CI/CD, VS Code integration, and cross-platform development.

The primary target is the Microchip SAMD51 (ARM Cortex-M4F) family, but the architecture supports adding other MCU families.

## Project Structure

```
firmware/
  common/                       Cross-MCU platform utilities
  samd51/
    common/                     SAMD51-family shared code (HAL wrappers)
    blinky/                     Demo blinky firmware project
boards/
  board_common/board_common.h   Shared board interface
  blinky_board/                 Example board (config, pins, main)
cmake/
  arm-gcc-toolchain.cmake       ARM cross-compilation toolchain
  samd51.cmake                  SAMD51 MCU support and add_board_firmware()
scripts/
  fetch_deps.sh                 Download Microchip SAMD51 DFP
  flash_openocd.sh              Flash via OpenOCD + Atmel-ICE
  new_board.sh                  Scaffold a new board directory
third_party/
  CMSIS_6/                      ARM CMSIS v6 (git submodule)
  samd51_dfp/                   Microchip SAMD51 Device Family Pack
```

Dependency chain: `fw_platform_common` &rarr; `samd51_common` &rarr; `fw_<project>` &rarr; `fw_<board>.elf`

## Quick Start

### Prerequisites

| Tool | Version | Purpose |
|------|---------|---------|
| `arm-none-eabi-gcc` | 13.x+ | ARM cross-compiler |
| `cmake` | 3.21+ | Build system generator |
| `ninja` | 1.12+ | Parallel build tool |

> **Windows users:** See [docs/getting-started.md](docs/getting-started.md) for full MSYS2 setup instructions.

### Build

```bash
git clone --recurse-submodules https://github.com/miniscope/miniscope-firmware.git
cd miniscope-firmware
bash scripts/fetch_deps.sh
cmake --preset blinky_board && cmake --build --preset blinky_board
```

Build outputs (`*.elf`, `*.hex`, `*.bin`, `*.map`) appear in `build/blinky_board/`.

## Adding a New Board

```bash
bash scripts/new_board.sh <name> [mcu_variant]
```

This scaffolds a board directory from the `blinky_board` template. Then:

1. Edit `boards/<name>/board_config.cmake` &mdash; set MCU variant, firmware project, feature flags
2. Edit `boards/<name>/board.h` and `board.c` for your hardware
3. Add `add_board_firmware(<name>)` in the root `CMakeLists.txt`
4. Add configure + build presets in `CMakePresets.json`

See [CLAUDE.md](CLAUDE.md) for full conventions.

## Adding a New Firmware Project

1. Create `firmware/<mcu>/<project>/` with `CMakeLists.txt`, `src/`, and `include/`
2. Define an INTERFACE library target (e.g., `fw_<project>`) linking against `samd51_common`
3. Add `add_subdirectory()` in the root `CMakeLists.txt`
4. Set `BOARD_FIRMWARE_PROJECT` in board configs that should use it

## Flash and Debug

### Flash

Connect an Atmel-ICE programmer via SWD, then:

```bash
bash scripts/flash_openocd.sh build/blinky_board/fw_blinky_board.elf
```

### Debug (VS Code)

The project includes launch configurations for the [Cortex-Debug](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug) extension with OpenOCD + Atmel-ICE (CMSIS-DAP/SWD).

Recommended VS Code extensions (auto-suggested via `.vscode/extensions.json`):

- [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) &mdash; configure/build integration
- [Cortex-Debug](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug) &mdash; on-chip debugging
- [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) &mdash; IntelliSense and code navigation

## CI/CD

GitHub Actions ([`.github/workflows/ci.yml`](.github/workflows/ci.yml)) runs on every push and PR:

- **Auto-discovers boards** from non-hidden, non-debug configure presets in `CMakePresets.json`
- **Builds each board** in a matrix (Release configuration)
- **Uploads artifacts** (`.elf`, `.hex`, `.bin`, `.map`) retained for 30 days
- **Creates GitHub Releases** with firmware binaries on `v*` tag pushes

No workflow changes are needed when adding a new board &mdash; just add a preset.

## Documentation

| Document | Description |
|----------|-------------|
| [docs/getting-started.md](docs/getting-started.md) | Windows/MSYS2 setup, build, flash, and debug walkthrough |
| [CLAUDE.md](CLAUDE.md) | Developer conventions, architecture details, and contribution guidelines |

## License

This project is licensed under the [GNU General Public License v3.0](LICENSE).
