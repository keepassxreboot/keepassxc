#!/usr/bin/env bash
#
# KeePassXC Browser Extension Native Messaging Installer Tool
# Copyright (C) 2017 KeePassXC team <https://keepassxc.org/>
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

set -e

JSON_OUT=""
BASE_DIR="."
INSTALL_DIR=""
INSTALL_FILE="org.keepassxc.keepassxc_browser.json"

# Early out if the keepassxc.proxy executable cannot be found
if ! command -v keepassxc.proxy; then
    echo "Could not find keepassxc.proxy! Ensure the keepassxc snap is installed properly."
    exit 0
fi

PROXY_PATH=$(command -v keepassxc.proxy)

JSON_FIREFOX=$(cat << EOF
{
    "name": "org.keepassxc.keepassxc_browser",
    "description": "KeePassXC integration with native messaging support",
    "path": "${PROXY_PATH}",
    "type": "stdio",
    "allowed_extensions": [
        "keepassxc-browser@keepassxc.org"
    ]
}
EOF
)

JSON_CHROME=$(cat << EOF
{
    "name": "org.keepassxc.keepassxc_browser",
    "description": "KeePassXC integration with native messaging support",
    "path": "${PROXY_PATH}",
    "type": "stdio",
    "allowed_origins": [
        "chrome-extension://iopaggbpplllidnfmcghoonnokmjoicf/",
        "chrome-extension://oboonakemofpalcgghocfoadofidjkkk/"
    ]
}
EOF
)

askBrowserSnap() {
    if (whiptail --title "Snap Choice" --defaultno \
            --yesno "Is this browser installed as a snap (usually NO)?" 8 60); then
        # BASE_DIR="$1"
        whiptail --title "Snap Choice" --msgbox "Sorry, browsers installed as snaps are not supported at this time" 8 50
        exit 0
    fi
}

setupFirefox() {
    askBrowserSnap "./snap/firefox/common"
    JSON_OUT=${JSON_FIREFOX}
    INSTALL_DIR="${BASE_DIR}/.mozilla/native-messaging-hosts"
}

setupChrome() {
    JSON_OUT=${JSON_CHROME}
    INSTALL_DIR="${BASE_DIR}/.config/google-chrome/NativeMessagingHosts"
}

setupChromium() {
    askBrowserSnap "./snap/chromium/current"
    JSON_OUT=${JSON_CHROME}
    INSTALL_DIR="${BASE_DIR}/.config/chromium/NativeMessagingHosts"
}

setupVivaldi() {
    JSON_OUT=${JSON_CHROME}
    INSTALL_DIR="${BASE_DIR}/.config/vivaldi/NativeMessagingHosts"
}

setupBrave() {
    JSON_OUT=${JSON_CHROME}
    INSTALL_DIR="${BASE_DIR}/.config/BraveSoftware/Brave-Browser/NativeMessagingHosts"
}

setupTorBrowser() {
    JSON_OUT=${JSON_FIREFOX}
    INSTALL_DIR="${BASE_DIR}/.tor-browser/app/Browser/TorBrowser/Data/Browser/.mozilla/native-messaging-hosts"
}

# --------------------------------
# Start of script
# --------------------------------

BROWSER=$(whiptail \
            --title "Browser Selection" \
            --menu "Choose a browser to integrate with KeePassXC:" \
            15 60 5 \
            "1" "Firefox" \
            "2" "Chrome" \
            "3" "Chromium" \
            "4" "Vivaldi" \
            "5" "Brave" \
            "6" "Tor Browser" \
            3>&1 1>&2 2>&3)

clear

exitstatus=$?
if [ $exitstatus = 0 ]; then
    # Configure settings for the chosen browser
    case "$BROWSER" in
        1) setupFirefox ;;
        2) setupChrome ;;
        3) setupChromium ;;
        4) setupVivaldi ;;
        5) setupBrave ;;
        6) setupTorBrowser ;;
    esac

    # Install the JSON file
    cd ~
    mkdir -p "$INSTALL_DIR"
    echo "$JSON_OUT" > ${INSTALL_DIR}/${INSTALL_FILE}

    whiptail \
        --title "Installation Complete" \
        --msgbox "You will need to restart your browser in order to connect to KeePassXC" \
        8 50
else
    whiptail --title "Installation Canceled" --msgbox "No changes were made to your system" 8 50
fi
