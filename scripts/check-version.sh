#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
CMAKE_VERSION="$(sed -nE 's/^project\([^)]* VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' "$ROOT/CMakeLists.txt" | head -1)"
PACKAGE_VERSION="$(python - "$ROOT/package/metadata.json" <<'PY'
import json
import sys
with open(sys.argv[1], encoding='utf-8') as handle:
    print(json.load(handle)['KPlugin']['Version'])
PY
)"

if [[ -z "$CMAKE_VERSION" || -z "$PACKAGE_VERSION" ]]; then
    echo "Unable to read project versions." >&2
    exit 1
fi

printf 'CMake version:    %s\n' "$CMAKE_VERSION"
printf 'Package version:  %s\n' "$PACKAGE_VERSION"

if [[ "$CMAKE_VERSION" != "$PACKAGE_VERSION" ]]; then
    echo "Version mismatch: update CMakeLists.txt and package/metadata.json together." >&2
    exit 1
fi

if git rev-parse --git-dir >/dev/null 2>&1; then
    tag="v$CMAKE_VERSION"
    if git show-ref --tags --verify --quiet "refs/tags/$tag"; then
        printf 'Git tag:           %s\n' "$tag"
    else
        echo "Git tag:           $tag (not present locally)"
    fi
fi

printf '%s\n' 'Version consistency check passed.'
