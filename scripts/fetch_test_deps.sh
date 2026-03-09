#!/usr/bin/env bash
# Download Unity test framework for host-based unit testing.
#
# Usage: bash scripts/fetch_test_deps.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
UNITY_DIR="$ROOT_DIR/third_party/Unity"
UNITY_VERSION="v2.6.0"
UNITY_URL="https://github.com/ThrowTheSwitch/Unity/archive/refs/tags/${UNITY_VERSION}.tar.gz"

if [ -f "$UNITY_DIR/src/unity.c" ]; then
    echo "Unity already present at $UNITY_DIR"
    exit 0
fi

echo "Downloading Unity ${UNITY_VERSION}..."
mkdir -p "$UNITY_DIR"

curl -sL "$UNITY_URL" | tar xz --strip-components=1 -C "$UNITY_DIR"

echo "Unity ${UNITY_VERSION} installed to $UNITY_DIR"
