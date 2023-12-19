#!/usr/bin/env bash

if [ "$1" == "cli" ] || [ "$(basename "$ARGV0")" == "keepassxc-cli" ] || [ "$(basename "$ARGV0")" == "keepassxc-cli.AppImage" ]; then
    [ "$1" == "cli" ] && shift
    exec keepassxc-cli "$@"
elif  [ "$1" == "proxy" ] || [ "$(basename "$ARGV0")" == "keepassxc-proxy" ] || [ "$(basename "$ARGV0")" == "keepassxc-proxy.AppImage" ] \\
    || [ -v CHROME_WRAPPER ] || [ -v MOZ_LAUNCHED_CHILD ]; then
    [ "$1" == "proxy" ] && shift
    exec keepassxc-proxy "$@"
else
    exec keepassxc "$@"
fi
