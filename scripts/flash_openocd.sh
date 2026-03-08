#!/usr/bin/env bash
# Flash firmware to a SAMD51 board via OpenOCD + Atmel-ICE (CMSIS-DAP).
#
# Usage:
#   ./scripts/flash_openocd.sh <path-to-elf>
#   ./scripts/flash_openocd.sh build/blinky_board/fw_blinky_board.elf
#
# Requires: openocd in PATH

set -euo pipefail

if [ $# -lt 1 ]; then
    echo "Usage: $0 <path-to-elf> [openocd-extra-args...]"
    exit 1
fi

ELF="$1"
shift

if [ ! -f "${ELF}" ]; then
    echo "ERROR: ELF file not found: ${ELF}"
    exit 1
fi

echo "Flashing ${ELF}..."

openocd \
    -f interface/cmsis-dap.cfg \
    -f target/atsame5x.cfg \
    "$@" \
    -c "init" \
    -c "reset halt" \
    -c "program ${ELF} verify reset exit"
