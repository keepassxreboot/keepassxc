#!/usr/bin/env bash

export PATH="$(dirname $0)/usr/bin:${PATH}"

if [ "$1" == "cli" ] || [ "$(basename "$ARGV0")" == "keepassxc-cli" ] || [ "$(basename "$ARGV0")" == "keepassxc-cli.AppImage" ]; then
    [ "$1" == "cli" ] && shift
    exec keepassxc-cli "$@"
elif [ "$1" == "proxy" ] || [ "$(basename "$ARGV0")" == "keepassxc-proxy" ] || [ "$(basename "$ARGV0")" == "keepassxc-proxy.AppImage" ]; then
    [ "$1" == "proxy" ] && shift
    exec keepassxc-proxy "$@"
elif [ -v CHROME_WRAPPER ] || [ -v MOZ_LAUNCHED_CHILD ] || [ "$2" == "keepassxc-browser@keepassxc.org" ]; then
    exec keepassxc-proxy "$@"
else
    # --- Icon file setup ---
    icon_source="$APPDIR/usr/share/icons/hicolor/256x256/apps/keepassxc.png"
    icon_target="${XDG_DATA_HOME:-$HOME/.local/share}/icons/hicolor/256x256/apps/keepassxc.png"
    mkdir -p "$(dirname "$icon_target")"

    # Copy icon if different or missing
    if [ ! -f "$icon_target" ] || ! cmp -s "$icon_source" "$icon_target"; then
        cp "$icon_source" "$icon_target"
    fi

    # --- Desktop file setup ---
    desktop_source="$APPDIR/usr/share/applications/org.keepassxc.KeePassXC.desktop"
    desktop_target="${XDG_DATA_HOME:-$HOME/.local/share}/applications/org.keepassxc.KeePassXC.desktop"
    if [ ! -f "$desktop_target" ]; then
        mkdir -p "$(dirname "$desktop_target")"
    fi

    # Substitute Exec and TryExec in memory
    desktop_content=$(sed "s|Exec=keepassxc %f|Exec=$APPIMAGE %f|;s|TryExec=keepassxc|TryExec=$APPIMAGE|" "$desktop_source")

    # Copy to target if different
    if ! cmp -s - "$desktop_target" <<<"$desktop_content"; then
        printf '%s\n' "$desktop_content" >"$desktop_target"
    fi

    EXEC="exec"
    if command -v systemd-run &>/dev/null; then
        EXEC="exec systemd-run --user --scope --slice=app.slice --unit=app-org.keepassxc.KeePassXC-$(cat /proc/sys/kernel/random/uuid | tr -d -).scope"
    fi

    $EXEC keepassxc "$@"
fi
