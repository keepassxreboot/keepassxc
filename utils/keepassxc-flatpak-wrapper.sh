#!/usr/bin/env bash
#
# Flatpak Multiple Commands Wrapper
# Copyright (C) 2022 KeePassXC team <https://keepassxc.org/>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 or (at your option)
# version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# This script is a workaround to the limitation of one command per Flatpak
# manifest. It solves this by redirecting stdio to keepassxc-proxy, as
# necessary, based upon matching command line arguments.

# For format of parsed arguments, see "Connection-based messaging" at:
# https://developer.mozilla.org/docs/Mozilla/Add-ons/WebExtensions/Native_messaging

# Chromium, Google Chrome, Vivaldi & Brave
readonly arg1='chrome-extension://oboonakemofpalcgghocfoadofidjkkk'
# Firefox & Tor Browser
readonly arg2='keepassxc-browser@keepassxc.org'

# Check arguments to see if this was a proxy launch from the browser
# Use =~ to account for minor variations in the chrome extension
if [[ "$1" =~ "$arg1" || "$2" == "$arg2" ]]; then
    exec keepassxc-proxy "$@"
elif [[ "$1" == "cli" ]]; then
    exec keepassxc-cli "${@:2}"
else
    # If no arguments are matched or browser integration is off, execute keepassxc
    exec keepassxc "$@"
fi
