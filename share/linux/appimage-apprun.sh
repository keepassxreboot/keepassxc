#!/usr/bin/env bash

export PATH="$(dirname $0)/usr/bin:${PATH}"

if [ "$1" == "cli" ] || [ "$(basename "$ARGV0")" == "keepassxc-cli" ] || [ "$(basename "$ARGV0")" == "keepassxc-cli.AppImage" ]; then
    [ "$1" == "cli" ] && shift
    exec keepassxc-cli "$@"
elif  [ "$1" == "proxy" ] || [ "$(basename "$ARGV0")" == "keepassxc-proxy" ] || [ "$(basename "$ARGV0")" == "keepassxc-proxy.AppImage" ]; then
    [ "$1" == "proxy" ] && shift
    exec keepassxc-proxy "$@"
elif [ -v CHROME_WRAPPER ] || [ -v MOZ_LAUNCHED_CHILD ] || [ "$2" == "keepassxc-browser@keepassxc.org" ]; then
    exec keepassxc-proxy "$@"
else
    exec keepassxc "$@"
fi
