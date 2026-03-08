# Getting Started

Local development setup for building Miniscope firmware.

## Prerequisites

- **Windows 10/11** with [MSYS2](https://www.msys2.org/) installed
- **Git** (included with MSYS2)

All commands below should be run in the **MSYS2 MINGW64** terminal.

## 1. Install Build Tools

```bash
pacman -S \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-arm-none-eabi-gcc \
  mingw-w64-x86_64-arm-none-eabi-newlib \
  mingw-w64-x86_64-openocd
```

This installs:

| Tool | Purpose |
|------|---------|
| `cmake` | Build system generator (≥3.21 required) |
| `ninja` | Fast parallel build tool |
| `arm-none-eabi-gcc` | ARM cross-compiler (Cortex-M) |
| `arm-none-eabi-newlib` | C library (`nosys.specs`, `nano.specs`, libc) |
| `openocd` | On-chip debugger for flashing/debugging |

> **Important:** Install the **mingw-w64-x86_64-** prefixed packages (not the plain `cmake`/`ninja` MSYS packages). The MSYS versions strip environment variables when spawning subprocesses, which causes GCC to fail with "Cannot create temporary file in C:\Windows\: Permission denied".

### Verify installation

```bash
arm-none-eabi-gcc --version   # 13.x
cmake --version               # 3.21+
ninja --version               # 1.12+
```

If `arm-none-eabi-gcc` is not found, ensure `/mingw64/bin` is on your PATH. The MINGW64 shell adds it automatically.

## 2. Clone and Fetch Dependencies

```bash
git clone --recurse-submodules https://github.com/<org>/miniscope-firmware.git
cd miniscope-firmware

# Download the Microchip SAMD51 Device Family Pack
bash scripts/fetch_deps.sh
```

If you already cloned without `--recurse-submodules`:

```bash
git submodule update --init
bash scripts/fetch_deps.sh
```

## 3. Build

```bash
# Configure
cmake --preset blinky_board

# Build
cmake --build --preset blinky_board
```

Build outputs appear in `build/blinky_board/`:

| File | Description |
|------|-------------|
| `fw_blinky_board.elf` | Linked firmware with debug symbols |
| `fw_blinky_board.hex` | Intel HEX for programming |
| `fw_blinky_board.bin` | Raw binary |
| `fw_blinky_board.map` | Linker map (memory layout) |

### Debug build

```bash
cmake --preset blinky_board-debug
cmake --build --preset blinky_board-debug
```

The debug build adds `-Og -g3 -gdwarf-2` flags for full debug symbols while keeping code debuggable.

## 4. Flash

Connect an Atmel-ICE programmer via SWD, then:

```bash
bash scripts/flash_openocd.sh build/blinky_board/fw_blinky_board.elf
```

## 5. Debug (VS Code)

1. Install the [Cortex-Debug](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug) extension
2. Open the project in VS Code
3. Press **F5** to start debugging (uses `.vscode/launch.json`)

The debug configuration uses OpenOCD with the Atmel-ICE (CMSIS-DAP/SWD interface).

## Troubleshooting

### "Cannot create temporary file in C:\Windows\: Permission denied"

You are using the MSYS versions of cmake/ninja instead of the MinGW versions. Reinstall:

```bash
# Remove MSYS versions
pacman -R cmake ninja

# Install MinGW versions
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja
```

Verify: `which cmake` should show `/mingw64/bin/cmake`, **not** `/usr/bin/cmake`.

### arm-none-eabi-gcc not found

Make sure you're using the **MINGW64** terminal (not the plain MSYS2 terminal). The MINGW64 terminal adds `/mingw64/bin` to PATH.

### Linker warnings about `_close`, `_read`, `_write`, `_lseek`

These are harmless. The `nosys.specs` stubs don't implement POSIX I/O, which is expected for bare-metal firmware. The linker notes they will be garbage-collected if unused.

### Stale build

If you change CMake files or switch presets, do a clean reconfigure:

```bash
rm -rf build/blinky_board
cmake --preset blinky_board
cmake --build --preset blinky_board
```
