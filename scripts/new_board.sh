#!/usr/bin/env bash
# Scaffold a new board from the blinky_board template.
#
# Usage: bash scripts/new_board.sh <board_name> [mcu_variant]
#
# Arguments:
#   board_name   - Name for the new board directory (e.g. "my_board")
#   mcu_variant  - MCU variant, lowercase (default: samd51p20a)
#
# Example:
#   bash scripts/new_board.sh sensor_board samd51j20a

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
TEMPLATE_DIR="$ROOT_DIR/boards/blinky_board"

BOARD_NAME="${1:-}"
MCU_VARIANT="${2:-samd51p20a}"

if [ -z "$BOARD_NAME" ]; then
    echo "Usage: bash scripts/new_board.sh <board_name> [mcu_variant]"
    echo "  mcu_variant defaults to samd51p20a"
    exit 1
fi

DEST_DIR="$ROOT_DIR/boards/$BOARD_NAME"

if [ -d "$DEST_DIR" ]; then
    echo "Error: $DEST_DIR already exists"
    exit 1
fi

# Copy template (main.c lives in the firmware project, not the board)
cp -r "$TEMPLATE_DIR" "$DEST_DIR"
rm -f "$DEST_DIR/main.c"

# Substitute MCU variant if different from the template default
if [ "$MCU_VARIANT" != "samd51p20a" ]; then
    MCU_UPPER=$(echo "$MCU_VARIANT" | tr '[:lower:]' '[:upper:]')

    # board_config.cmake: update MCU, device define, and linker script
    sed -i "s/samd51p20a/${MCU_VARIANT}/g" "$DEST_DIR/board_config.cmake"
    sed -i "s/__SAMD51P20A__/__${MCU_UPPER}__/g" "$DEST_DIR/board_config.cmake"

    # board.h: update comment
    sed -i "s/ATSAMD51P20A/AT${MCU_UPPER}/g" "$DEST_DIR/board.h"
fi

echo "Created boards/$BOARD_NAME/ from template"
echo ""
echo "Remaining steps:"
echo "  1. Edit boards/$BOARD_NAME/board_config.cmake (set firmware project, pins, etc.)"
echo "  2. Edit boards/$BOARD_NAME/board.h and board.c for your hardware"
echo "  3. Add to root CMakeLists.txt:"
echo "       add_board_firmware($BOARD_NAME)"
echo "  4. Add presets to CMakePresets.json:"
echo "       {\"name\": \"$BOARD_NAME\", \"displayName\": \"...\", \"inherits\": \"base\"}"
echo "       {\"name\": \"${BOARD_NAME}-debug\", \"displayName\": \"... - Debug\", \"inherits\": \"base\","
echo "        \"cacheVariables\": {\"CMAKE_BUILD_TYPE\": \"Debug\"}}"
echo "  5. CI will auto-discover the new preset — no workflow changes needed"
