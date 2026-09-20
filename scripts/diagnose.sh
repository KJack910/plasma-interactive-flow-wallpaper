#!/usr/bin/env bash
set -u
printf '%s\n' '=== XMB Native Wallpaper diagnostics ==='
printf 'Session: '; echo "${XDG_SESSION_TYPE:-unknown}"
printf 'Desktop: '; echo "${XDG_CURRENT_DESKTOP:-unknown}"
printf 'Plasma: '; plasmashell --version 2>/dev/null || true
printf 'Qt: '; qmake6 -query QT_VERSION 2>/dev/null || true
printf '\nInstalled package:\n'
kpackagetool6 --type Plasma/Wallpaper --list 2>/dev/null | grep -F 'org.xmbflow.interactive' || true
printf '\nNative library:\n'
find "$HOME/.local/share/plasma/wallpapers/org.xmbflow.interactive" -name 'libxmbnativeplugin.so*' -ls 2>/dev/null || true
printf '\nRecent plasmashell XMB/QML errors:\n'
journalctl --user -u plasma-plasmashell.service -n 200 --no-pager 2>/dev/null | grep -Ei 'xmb|qml|opengl|shader|plugin' | tail -80 || true
