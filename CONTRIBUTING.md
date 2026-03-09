# Contributing to Miniscope Firmware

## Getting Started

```bash
# Clone with submodules
git clone --recursive https://github.com/miniscope/miniscope-firmware.git
cd miniscope-firmware

# Fetch vendor dependencies
bash scripts/fetch_deps.sh

# Build a board
cmake --preset blinky_board
cmake --build --preset blinky_board
```

### Prerequisites

- `arm-none-eabi-gcc` (13.x recommended)
- `cmake` (>= 3.21)
- `ninja`

## Code Style

This project uses [clang-format](.clang-format) for C source formatting:

- LLVM-based style, 4-space indent, 100-column limit
- Linux brace style (opening brace on same line for functions)
- Includes are **not** auto-sorted (embedded include order matters)

Format your changes before committing:

```bash
clang-format -i <files>
```

## Running Tests

Host-based unit tests run on your machine without cross-compilation:

```bash
bash scripts/fetch_test_deps.sh
cmake --preset host-tests
cmake --build --preset host-tests
ctest --preset host-tests
```

## Adding a New Board

```bash
bash scripts/new_board.sh <board_name> [mcu_variant]
```

Then:
1. Edit `boards/<name>/board_config.cmake` for your MCU, pins, and features
2. Edit `boards/<name>/board.h` and `board.c` for hardware init
3. Add `add_board_firmware(<name>)` in root `CMakeLists.txt`
4. Add configure + build presets in `CMakePresets.json`

CI auto-discovers boards from non-hidden, non-debug configure presets.

## Adding a New Firmware Project

1. Create `firmware/<mcu>/<project>/` with `CMakeLists.txt`, `src/`, `include/`
2. Define an INTERFACE library target (e.g., `fw_<project>`) linking against `samd51_common`
3. Expose sources via `INTERFACE_FW_SOURCES` property
4. Add `add_subdirectory()` in root `CMakeLists.txt`
5. Set `BOARD_FIRMWARE_PROJECT` in board configs that should use it

## Commit Messages

Use imperative mood, present tense:

- **Good:** "Add SPI driver for SAMD51"
- **Bad:** "Added SPI driver" / "Adds SPI driver"

Keep the first line under 72 characters. Add a blank line then a longer
description if needed.

## Pull Request Guidelines

1. One logical change per PR
2. Ensure cross-compiled builds pass (`cmake --preset blinky_board && cmake --build --preset blinky_board`)
3. Ensure host tests pass (`ctest --preset host-tests`)
4. Add tests for new application logic where feasible
5. Update documentation if adding new HAL functions or board interfaces
