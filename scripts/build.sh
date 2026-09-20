#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build"

# Always regenerate AUTOMOC/QML metadata from the current sources.
rm -rf "$BUILD"

cmake -S "$ROOT" -B "$BUILD" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" --parallel "$(nproc)"

echo
echo "Build completata. Plugin nativo:"
find "$ROOT/package/contents/ui/xmbnative" -maxdepth 1 -name 'libxmbnativeplugin.so*' -print
