#!/usr/bin/env bash
# ZION - one-click build + run (macOS)
# Needs: Xcode command-line tools, CMake, git.
set -euo pipefail
cd "$(dirname "$0")"

if ! command -v cmake >/dev/null; then
    echo "[error] cmake not found. Install with: brew install cmake"
    exit 1
fi

if [[ -z "${VCPKG_ROOT:-}" || ! -x "${VCPKG_ROOT:-}/vcpkg" ]]; then
    if [[ -x "$HOME/vcpkg/vcpkg" ]]; then
        export VCPKG_ROOT="$HOME/vcpkg"
    else
        echo "[setup] Installing vcpkg into $HOME/vcpkg ..."
        git clone https://github.com/microsoft/vcpkg "$HOME/vcpkg"
        "$HOME/vcpkg/bootstrap-vcpkg.sh" -disableMetrics
        export VCPKG_ROOT="$HOME/vcpkg"
    fi
fi
echo "[setup] Using vcpkg at $VCPKG_ROOT"

echo "[build] Configuring (first run also builds raylib - takes a few minutes) ..."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

echo "[build] Compiling ..."
cmake --build build --parallel

echo "[run] build/zion"
exec ./build/zion "$@"
