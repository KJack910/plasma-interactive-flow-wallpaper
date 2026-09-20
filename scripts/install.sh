#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
PLUGIN_ID="org.xmbflow.interactive"
KWIN_SCRIPT_ID="xmbfullscreenbridge"

if [[ "${1:-}" == "--deps" ]]; then
  sudo pacman -S --needed \
    base-devel cmake ninja qt6-base qt6-declarative \
    qt6-tools extra-cmake-modules libplasma kf6-kpackage
fi

for cmd in cmake ninja kpackagetool6; do
  if ! command -v "$cmd" >/dev/null 2>&1; then
    echo "Errore: comando '$cmd' non trovato." >&2
    echo "Su CachyOS esegui: $0 --deps" >&2
    exit 1
  fi
done

"$ROOT/scripts/build.sh"

SO="$ROOT/package/contents/ui/xmbnative/libxmbnativeplugin.so"
if [[ ! -f "$SO" ]]; then
  echo "Errore: il plugin nativo non è stato generato: $SO" >&2
  exit 1
fi

# Remove the previous WebEngine prototype / stale package before installing the
# native build. This intentionally preserves Plasma's containment settings.
kpackagetool6 --type Plasma/Wallpaper --remove "$PLUGIN_ID" >/dev/null 2>&1 || true
rm -rf "$HOME/.local/share/plasma/wallpapers/$PLUGIN_ID"

kpackagetool6 --type Plasma/Wallpaper --install "$ROOT/package"

echo
echo "XMB Interactive Flow nativo installato."
echo "Riavvio plasmashell..."
systemctl --user restart plasma-plasmashell.service || {
  echo "Riavvio automatico non riuscito. Esegui manualmente:"
  echo "  systemctl --user restart plasma-plasmashell.service"
}

# Reload KWin only after plasmashell owns the DBus bridge. This makes KWin's
# initial window scan reach the newly created wallpaper instances.
kpackagetool6 --type KWin/Script --remove "$KWIN_SCRIPT_ID" >/dev/null 2>&1 || true
kpackagetool6 --type KWin/Script --install "$ROOT/kwin-script"
kwriteconfig6 --file kwinrc --group Plugins --key "${KWIN_SCRIPT_ID}Enabled" true
qdbus6 org.kde.KWin /KWin reconfigure >/dev/null 2>&1 || true
qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.unloadScript "$KWIN_SCRIPT_ID" >/dev/null 2>&1 || true
qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.loadScript \
  "$HOME/.local/share/kwin/scripts/$KWIN_SCRIPT_ID/contents/code/main.js" \
  "$KWIN_SCRIPT_ID" >/dev/null 2>&1 || true
qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.start >/dev/null 2>&1 || true

echo
echo "Poi: tasto destro desktop → Configura desktop e sfondo → XMB Interactive Flow"
