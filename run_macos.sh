#!/usr/bin/env bash
# ==============================================================
#  TacMove - build + run (macOS, Apple Silicon & Intel)
#  Needs: git and the Xcode Command Line Tools
#  (xcode-select --install). No Homebrew required: if cmake is
#  not on PATH, the script fetches a private copy through vcpkg.
#  First run clones vcpkg to ~/vcpkg and compiles SDL3 from
#  source - expect a few minutes.
# ==============================================================
set -euo pipefail
cd "$(dirname "$0")"

if ! xcode-select -p >/dev/null 2>&1; then
    echo "[setup] Xcode Command Line Tools are required. Install with:"
    echo "        xcode-select --install"
    exit 1
fi

if [ -z "${VCPKG_ROOT:-}" ]; then
    VCPKG_ROOT="$HOME/vcpkg"
fi
if [ ! -x "$VCPKG_ROOT/vcpkg" ]; then
    if [ ! -d "$VCPKG_ROOT" ]; then
        echo "[setup] Installing vcpkg into $VCPKG_ROOT ..."
        git clone https://github.com/microsoft/vcpkg "$VCPKG_ROOT"
    fi
    "$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics
fi
export VCPKG_ROOT
echo "[setup] Using vcpkg at $VCPKG_ROOT"

# Use the cmake on PATH if there is one; otherwise have vcpkg download
# its own copy (kept inside $VCPKG_ROOT/downloads, nothing installed
# system-wide).
if command -v cmake >/dev/null 2>&1; then
    CMAKE=cmake
else
    echo "[setup] cmake not found on PATH - fetching one through vcpkg ..."
    CMAKE="$("$VCPKG_ROOT/vcpkg" fetch cmake | tail -n1)"
fi
echo "[setup] Using cmake: $CMAKE"

# pkg-config: the SDL3 port's post-build step needs one, and vcpkg does not
# download it on macOS. If none is on PATH (no Homebrew), build vcpkg's own
# pkgconf (takes ~15 s) and hand it over via PKG_CONFIG.
if [ -z "${PKG_CONFIG:-}" ] && ! command -v pkg-config >/dev/null 2>&1; then
    if [ "$(uname -m)" = "arm64" ]; then HOST_TRIPLET=arm64-osx; else HOST_TRIPLET=x64-osx; fi
    PKGCONF="$VCPKG_ROOT/installed/$HOST_TRIPLET/tools/pkgconf/pkgconf"
    if [ ! -x "$PKGCONF" ]; then
        echo "[setup] pkg-config not found on PATH - building pkgconf through vcpkg ..."
        (cd "$VCPKG_ROOT" && ./vcpkg install pkgconf --triplet "$HOST_TRIPLET")
    fi
    export PKG_CONFIG="$PKGCONF"
    echo "[setup] Using pkg-config: $PKG_CONFIG"
fi

echo "[build] Configuring (first run also builds SDL3 - takes a few minutes) ..."
"$CMAKE" -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

echo "[build] Compiling ..."
"$CMAKE" --build build --parallel "$(sysctl -n hw.ncpu)"

./build/tacmove "$@"
