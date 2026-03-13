#!/usr/bin/env bash
# Fetch third-party dependencies for building miniscope-firmware.
#
# Downloads:
#   - Microchip SAMD51 DFP (device headers, startup code, linker scripts, SVD)
#
# CMSIS_6 is managed as a git submodule and is NOT fetched by this script.
# Run: git submodule update --init
#
# Usage:
#   ./scripts/fetch_deps.sh
#
# Re-running is safe; it will skip downloads if the target directory exists.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
THIRD_PARTY="${REPO_ROOT}/third_party"

# ---------------------------------------------------------------------------
# SAMD51 DFP
# ---------------------------------------------------------------------------
DFP_VERSION="3.8.246"
DFP_SHA256="6d2f4e82d0e97bd53e92a68aaea2811aed620dab597045666d25ddd82e885935"
DFP_FILENAME="Microchip.SAMD51_DFP.${DFP_VERSION}.atpack"
DFP_URL="https://packs.download.microchip.com/${DFP_FILENAME}"
DFP_DIR="${THIRD_PARTY}/samd51_dfp"

if [ -d "${DFP_DIR}/samd51a/include" ]; then
    echo "SAMD51 DFP already present at ${DFP_DIR}, skipping."
else
    echo "Downloading SAMD51 DFP v${DFP_VERSION}..."
    mkdir -p "${DFP_DIR}"

    TMPFILE=$(mktemp)
    trap "rm -f ${TMPFILE}" EXIT

    if command -v curl &>/dev/null; then
        curl -fSL "${DFP_URL}" -o "${TMPFILE}"
    elif command -v wget &>/dev/null; then
        wget -q "${DFP_URL}" -O "${TMPFILE}"
    else
        echo "ERROR: Neither curl nor wget found. Install one and retry."
        exit 1
    fi

    # Verify checksum
    echo "Verifying SHA256 checksum..."
    if command -v sha256sum &>/dev/null; then
        echo "${DFP_SHA256}  ${TMPFILE}" | sha256sum -c - || {
            echo "ERROR: SHA256 checksum mismatch!"
            exit 1
        }
    else
        echo "WARNING: sha256sum not available, skipping checksum verification."
    fi

    echo "Extracting DFP to ${DFP_DIR}..."
    unzip -qo "${TMPFILE}" -d "${DFP_DIR}"

    rm -f "${TMPFILE}"
    trap - EXIT

    echo "SAMD51 DFP v${DFP_VERSION} installed."
fi

# ---------------------------------------------------------------------------
# Verify
# ---------------------------------------------------------------------------
echo ""
echo "Dependency check:"
if [ -f "${DFP_DIR}/samd51a/include/samd51p20a.h" ]; then
    echo "  [OK] SAMD51 DFP headers"
else
    echo "  [!!] SAMD51 DFP headers NOT found - check DFP_VERSION or URL"
fi

if [ -f "${THIRD_PARTY}/CMSIS_6/CMSIS/Core/Include/core_cm4.h" ]; then
    echo "  [OK] CMSIS Core headers (via submodule)"
else
    echo "  [!!] CMSIS Core headers NOT found - run: git submodule update --init"
fi

if [ -f "${THIRD_PARTY}/tinyusb/src/tusb.h" ]; then
    echo "  [OK] TinyUSB (via submodule)"
else
    echo "  [!!] TinyUSB NOT found - run: git submodule update --init"
fi

echo ""
echo "Done."
