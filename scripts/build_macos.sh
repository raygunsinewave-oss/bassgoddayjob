#!/usr/bin/env bash
# One-command build for macOS: universal (arm64 + x86_64) VST3, AU and Standalone, plus tests.
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" "$@"
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
echo "Artefacts: build/BASSGODSDAYJOB_artefacts/Release/{VST3,AU,Standalone}"
