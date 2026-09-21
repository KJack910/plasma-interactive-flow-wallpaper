#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

command -v kscreen-doctor >/dev/null 2>&1 || {
    echo "kscreen-doctor is required; run this script inside an active Plasma session." >&2
    exit 1
}

kscreen-doctor -o | python3 "$ROOT/scripts/kscreen_topology.py"
