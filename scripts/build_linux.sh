#!/usr/bin/env bash
# Linux build (VST3 + Standalone) - used for development and CI tests.
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release "$@"
cmake --build build --parallel
ctest --test-dir build --output-on-failure
