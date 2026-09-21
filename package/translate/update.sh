#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
TEMPLATE="$ROOT/package/translate/template.pot"
CATALOG="$ROOT/package/translate/it.po"

command -v xgettext >/dev/null 2>&1 || {
    echo "xgettext is required to extract translation messages." >&2
    exit 1
}
command -v msgmerge >/dev/null 2>&1 || {
    echo "msgmerge is required to update translation catalogs." >&2
    exit 1
}

xgettext \
    --language=JavaScript \
    --from-code=UTF-8 \
    --keyword=i18n:1 \
    --keyword=i18np:1,2 \
    --package-name="plasma-interactive-flow-wallpaper" \
    --package-version="1.1.0" \
    --output="$TEMPLATE" \
    "$ROOT/package/contents/ui/config.qml"

msgmerge --update --no-fuzzy-matching "$CATALOG" "$TEMPLATE"
printf 'Updated %s from %s\n' "$CATALOG" "$TEMPLATE"
