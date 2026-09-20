#!/usr/bin/env bash
set -euo pipefail
echo "=== KDE output topology ==="
if ! kscreen-doctor -o; then
    echo "Impossibile interrogare KScreen: esegui questo script nella sessione Plasma/Wayland attiva." >&2
    exit 1
fi
echo
echo "Expected for the topology supplied during v1.1.0 development:"
echo "  DP-1: X=0..1920, width=1920"
echo "  DP-2: X=1920..4480, width=2560"
echo "  Virtual desktop: 4480x1080"
echo "  Shared seam: world X = 1920"
echo "  Phase reference width: 1920"
echo
echo "Enable Multischermo + Diagnostica in the wallpaper settings."
echo "The strong grid lines must remain continuous across every horizontal or vertical seam."
