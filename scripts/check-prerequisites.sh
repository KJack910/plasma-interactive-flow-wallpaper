#!/usr/bin/env bash
set -u

missing=0

check_command() {
    local command_name="$1"
    if command -v "$command_name" >/dev/null 2>&1; then
        printf 'OK      %-18s %s\n' "$command_name" "$(command -v "$command_name")"
    else
        printf 'MISSING %-18s\n' "$command_name"
        missing=1
    fi
}

printf '%s\n' '=== XMB Native prerequisites ==='
printf 'OS: %s\n' "$(uname -s)"
printf 'Desktop: %s\n' "${XDG_CURRENT_DESKTOP:-unknown}"
printf 'Session: %s\n' "${XDG_SESSION_TYPE:-unknown}"
printf '\nBuild tools:\n'
check_command cmake
check_command ninja
check_command c++

printf '\nQt/KDE tools:\n'
check_command qmake6
check_command qmllint
check_command kpackagetool6
check_command plasmashell
check_command kwriteconfig6
check_command qdbus6

printf '\nTranslation tools:\n'
check_command msgfmt
check_command xgettext

printf '\nOptional diagnostics:\n'
for command_name in kscreen-doctor systemctl journalctl; do
    if command -v "$command_name" >/dev/null 2>&1; then
        printf 'AVAILABLE %-15s\n' "$command_name"
    else
        printf 'OPTIONAL  %-15s\n' "$command_name"
    fi
done

printf '\n'
if (( missing != 0 )); then
    printf '%s\n' 'Prerequisites incomplete. Install the missing build/KDE tools, then retry.' >&2
    exit 1
fi
printf '%s\n' 'Required command checks passed.'
