#!/usr/bin/env bash
# Wasm build via CMake + Emscripten. Requires: source emsdk_env.sh (see README).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

if ! command -v emcmake >/dev/null 2>&1; then
  echo "emcmake not found. Install Emscripten and run: source /path/to/emsdk/emsdk_env.sh" >&2
  exit 1
fi

mkdir -p wasmbuild
BUILD_DIR="${WASMGL_BUILD_WASM_DIR:-${ROOT}/build-wasm}"
emcmake cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j"$(nproc 2>/dev/null || echo 4)"
echo "Output: wasmbuild/wasmGL.js and wasmbuild/wasmGL.wasm"
echo "Run:   emrun --port 8080 wasmbuild/mypage.html"
