#!/usr/bin/env bash
set -euo pipefail
PLUGIN_ID="org.xmbflow.interactive"
KWIN_SCRIPT_ID="xmbfullscreenbridge"
kpackagetool6 --type Plasma/Wallpaper --remove "$PLUGIN_ID" || true
kpackagetool6 --type KWin/Script --remove "$KWIN_SCRIPT_ID" || true
kwriteconfig6 --file kwinrc --group Plugins --key "${KWIN_SCRIPT_ID}Enabled" false
qdbus6 org.kde.KWin /KWin reconfigure >/dev/null 2>&1 || true
rm -rf "$HOME/.local/share/plasma/wallpapers/$PLUGIN_ID"
systemctl --user restart plasma-plasmashell.service || true
echo "Rimosso $PLUGIN_ID"
