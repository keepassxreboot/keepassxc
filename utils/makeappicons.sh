#!/usr/bin/env bash
#
# Copy icon files from the Material Design icon set.
#
# Copyright (C) 2020 Wolfram Rösler
# Copyright (C) 2020 KeePassXC team <https://keepassxc.org/>
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
#
# How to use: (assuming you check out stuff in ~/src)
#
# 0. Make sure to have a clean working tree
#
# 1. Download the Material Design icon set:
#       $ cd ~/src
#       $ git clone https://github.com/Templarian/MaterialDesign.git
#
# 2. Go to the icon source directory:
#       $ cd ~/src/keepassxc/share/icons
#
# 3. Create the icons:
#       $ bash ../../utils/makeicons.sh ~/src/MaterialDesign
#
# 4. Re-build and run KeePassXC:
#       $ cd ~/keepassxc/build
#       $ make keepassxc && src/keepassxc
#
# Material icons: https://materialdesignicons.com/

NC='\033[0m'
RED='\033[0;31m'
YELLOW='\033[0;33m'

if [[ $# != 1 ]]; then
    echo "Usage: $0 MATERIAL"
    echo "MATERIAL is the directory containing the material icons repository"
    echo "(git clone https://github.com/Templarian/MaterialDesign.git)".
    exit
fi

MATERIAL=$1
if [[ ! -d $MATERIAL ]]; then
    echo -e "${RED}Material Icons directory does not exist: ${MATERIAL}${NC}"
    exit 1
fi

# Check destination dir
DSTDIR=share/icons/application
if [[ ! -d $DSTDIR ]]; then
    echo -e "${RED}Please invoke this script from the KeePassXC source root directory.${NC}"
    exit 1
fi

# Map KeePassXC icon names to Material icon names.
# $1 is the name of the icon file in the KeePassXC source (without
# path and without extension, e. g. "document-new").
# Writes the name of the Material icon (without path and without
# extension, e. g. "folder-plus") to stdout.
# If the icon name is unknown, outputs nothing.
map() {
    case $1 in
        application-exit)                   echo exit-run                       ;;
        auto-type)                          echo keyboard-variant               ;;
        bugreport)                          echo bug-outline                    ;;
        chronometer)                        echo clock-outline                  ;;
        clipboard-text)                     echo clipboard-text-outline         ;;
        configure)                          echo cog-outline                    ;;
        database-change-key)                echo key                            ;;
        database-close)                     echo close                          ;;
        database-lock)                      echo lock-outline                   ;;
        database-merge)                     echo merge                          ;;
        dialog-close)                       echo close                          ;;
        dialog-error)                       echo alert-circle-outline           ;;
        dialog-information)                 echo information-outline            ;;
        dialog-ok)                          echo checkbox-marked-circle         ;;
        dialog-warning)                     echo alert-outline                  ;;
        document-close)                     echo close                          ;;
        document-edit)                      echo pencil                         ;;
        document-export)                    echo export                         ;;
        document-import)                    echo import                         ;;
        document-new)                       echo plus                           ;;
        document-open)                      echo folder-open-outline            ;;
        document-open-recent)               echo folder-clock-outline           ;;
        document-properties)                echo file-edit-outline              ;;
        document-save)                      echo content-save-outline           ;;
        document-save-as)                   echo content-save-all-outline       ;;
        document-save-copy)                 echo content-save-move-outline      ;;
        donate)                             echo gift-outline                   ;;
        edit-clear-locationbar-ltr)         echo backspace-reverse-outline      ;;
        edit-clear-locationbar-rtl)         echo backspace-outline              ;;
        entry-clone)                        echo plus-circle-multiple-outline   ;;
        entry-delete)                       echo close-circle-outline           ;;
        entry-edit)                         echo pencil-circle-outline          ;;
        entry-new)                          echo plus-circle-outline            ;;
        favicon-download)                   echo download                       ;;
        getting-started)                    echo lightbulb-on-outline           ;;
        group-delete)                       echo folder-remove-outline          ;;
        group-edit)                         echo folder-edit-outline            ;;
        group-empty-trash)                  echo trash-can-outline              ;;
        group-new)                          echo folder-plus-outline            ;;
        health)                             echo heart-pulse                    ;;
        help-about)                         echo information-outline            ;;
        internet-web-browser)               echo web                            ;;
        keyboard-shortcuts)                 echo apple-keyboard-command         ;;
        key-enter)                          echo keyboard-variant               ;;
        message-close)                      echo close                          ;;
        move-down)                          echo chevron-double-down            ;;
        move-up)                            echo chevron-double-up              ;;
        object-locked)                      echo lock-outline                   ;;
        object-unlocked)                    echo lock-open-variant-outline      ;;
        paperclip)                          echo paperclip                      ;;
        password-copy)                      echo key-arrow-right                ;;
        password-generate)                  echo dice-3-outline                 ;;
        password-generator)                 echo dice-3-outline                 ;;
        password-show-off)                  echo eye-off-outline                ;;
        password-show-on)                   echo eye-outline                    ;;
        preferences-desktop-icons)          echo emoticon-happy-outline         ;;
        preferences-other)                  echo file-document-edit-outline     ;;
        preferences-system-network-sharing) echo lan                            ;;
        refresh)                            echo refresh                        ;;
        reports)                            echo lightbulb-on-outline           ;;
        reports-exclude)                    echo lightbulb-off-outline          ;;
        security-high)                      echo shield-outline                 ;;
        sort-alphabetical-ascending)        echo sort-alphabetical-ascending    ;;
        sort-alphabetical-descending)       echo sort-alphabetical-descending   ;;
        statistics)                         echo chart-line                     ;;
        system-help)                        echo help                           ;;
        system-search)                      echo magnify                        ;;
        system-software-update)             echo cloud-download-outline         ;;
        url-copy)                           echo earth-arrow-right              ;;
        user-guide)                         echo book-open-outline              ;;
        username-copy)                      echo account-arrow-right-outline    ;;
        utilities-terminal)                 echo console-line                   ;;
        view-history)                       echo timer-sand-empty               ;;
        web)                                echo web                            ;;
    esac
}

# Now do the actual work
find "$DSTDIR" -type f -name "*.svg" | while read -r DST; do

    # Find the icon name (base name without extender)
    NAME=$(basename "$DST" .svg)

    # Find the base name of the svg file for this icon
    MAT=$(map "$NAME")
    if [[ -z $MAT ]]; then
        echo -e "${YELLOW}Warning: No MaterialDesign mapping for ${NAME}${NC}"
        continue
    fi

    # So the source file is:
    SRC="$MATERIAL/svg/$MAT.svg"
    if [[ ! -f $SRC ]]; then
        echo -e "${RED}Error: Source for ${NAME} doesn't exist: ${SRC}${NC}"
        continue
    fi

    # Replace the icon file with the source file
    cp -- "$SRC" "$DST" || exit
    echo "Copied icon for ${NAME}"

done
