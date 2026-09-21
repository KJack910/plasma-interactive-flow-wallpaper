#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
DOMAIN="plasma_wallpaper_org.xmbflow.interactive"
SOURCE="$ROOT/package/translate/it.po"
OUTPUT="$ROOT/package/contents/locale/it/LC_MESSAGES/$DOMAIN.mo"

command -v msgfmt >/dev/null 2>&1 || {
    echo "msgfmt is required to build translations." >&2
    exit 1
}

mkdir -p "$(dirname -- "$OUTPUT")"
msgfmt --check -o "$OUTPUT" "$SOURCE"
printf 'Built %s\n' "$OUTPUT"
