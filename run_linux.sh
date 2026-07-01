#!/usr/bin/env bash
# ==============================================================
#  TacMove - build + run (Linux x64)
#  Needs: git, cmake >= 3.21, a C++20 compiler, and the system
#  dev packages listed in README.md (SDL3 is compiled from
#  source by vcpkg and links against X11/Wayland).
#  First run clones vcpkg to ~/vcpkg - expect a few minutes.
# ==============================================================
set -euo pipefail
cd "$(dirname "$0")"

if [ -z "${VCPKG_ROOT:-}" ]; then
    if [ -x "$HOME/vcpkg/vcpkg" ]; then
        VCPKG_ROOT="$HOME/vcpkg"
    else
        echo "[setup] Installing vcpkg into $HOME/vcpkg ..."
        git clone https://github.com/microsoft/vcpkg "$HOME/vcpkg"
        "$HOME/vcpkg/bootstrap-vcpkg.sh" -disableMetrics
        VCPKG_ROOT="$HOME/vcpkg"
    fi
fi
export VCPKG_ROOT
echo "[setup] Using vcpkg at $VCPKG_ROOT"

echo "[build] Configuring (first run also builds SDL3 - takes a few minutes) ..."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

echo "[build] Compiling ..."
cmake --build build --parallel

./build/tacmove "$@"
