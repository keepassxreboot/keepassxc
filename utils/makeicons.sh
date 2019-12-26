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
# 4. Re-build KeePassXC:
#       $ cd ~/keepassxc/build
#       $ make keepassxc
#
# 5. Check icons by disabling the OS icon theme:
#       $ KEEPASSXC_IGNORE_ICON_THEME=1 src/keepassxc
#
# Material icons: https://materialdesignicons.com/

if [ $# != 1 ];then
    echo "Usage: $0 MATERIAL"
    echo "MATERIAL is the check-out directory of the material icons repository"
    echo "(git clone https://github.com/Templarian/MaterialDesign.git)".
    exit
fi

MATERIAL="$1"
if [ ! -d "$MATERIAL" ];then
    echo "Material check-out directory doesn't exist: $MATERIAL"
    exit 1
fi

if [ ! -d application ];then
    echo "Please run this script from within the share/icons directory"
    echo "of the KeePassXC source distribution."
    exit 1
fi

# Map KeePassXC icon names to Material icon names.
# $1 is the name of the icon file in the KeePassXC source (without
# path and without extension, e. g. "document-new").
# Writes the name of the Material icon (without path and without
# extension, e. g. "folder-plus") to stdout.
# If the icon name is unknown, outputs nothing.
map() {
    case "$1" in
        application-exit)                   echo exit-to-app                    ;;
        auto-type)                          echo keyboard-variant               ;;
        bugreport)                          echo bug-outline                    ;;
        chronometer)                        echo clock-outline                  ;;
        configure)                          echo settings-outline               ;;
        database-change-key)                echo key                            ;;
        database-close)                     echo close                          ;;
        database-lock)                      echo lock-outline                   ;;
        database-merge)                     echo merge                          ;;
        dialog-close)                       echo close                          ;;
        dialog-error)                       echo alert-octagon                  ;;
        dialog-information)                 echo information-outline            ;;
        dialog-ok)                          echo checkbox-marked-circle         ;;
        dialog-warning)                     echo alert-outline                  ;;
        document-close)                     echo close                          ;;
        document-edit)                      echo pencil                         ;;
        document-new)                       echo plus                           ;;
        document-open)                      echo folder-open-outline            ;;
        document-properties)                echo file-edit-outline              ;;
        document-save)                      echo content-save-outline           ;;
        document-save-as)                   echo content-save-all-outline       ;;
        donate)                             echo gift-outline                   ;;
        edit-clear-locationbar-ltr)         echo backspace-reverse-outline      ;;
        edit-clear-locationbar-rtl)         echo backspace-outline              ;;
        entry-clone)                        echo comment-multiple-outline       ;;
        entry-delete)                       echo comment-remove-outline         ;;
        entry-edit)                         echo comment-edit-outline           ;;
        entry-new)                          echo comment-plus-outline           ;;
        favicon-download)                   echo download                       ;;
        getting-started)                    echo lightbulb-on-outline           ;;
        group-delete)                       echo folder-remove-outline          ;;
        group-edit)                         echo folder-edit-outline            ;;
        group-empty-trash)                  echo trash-can-outline              ;;
        group-new)                          echo folder-plus-outline            ;;
        help-about)                         echo information-outline            ;;
        internet-web-browser)               echo web                            ;;
        key-enter)                          echo keyboard-variant               ;;
        keyboard-shortcuts)                 echo apple-keyboard-command         ;;
        message-close)                      echo close                          ;;
        object-locked)                      echo lock-outline                   ;;
        object-unlocked)                    echo lock-open-variant-outline      ;;
        paperclip)                          echo paperclip                      ;;
        password-copy)                      echo key-arrow-right                ;;
        password-generate)                  echo dice-3-outline                 ;;
        password-generator)                 echo dice-3-outline                 ;;
        password-show-off)                  echo eye-off-outline                ;;
        password-show-on)                   echo eye-outline                    ;;
        preferences-other)                  echo file-document-edit-outline     ;;
        preferences-desktop-icons)          echo emoticon-happy-outline         ;;
        preferences-system-network-sharing) echo lan                            ;;
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
find application -type f -name "*.svg" | while read -r DST;do

    # Find the icon name (base name without extender)
    NAME=$(basename $DST .svg)

    # Find the base name of the svg file for this icon
    MAT=$(map $NAME)
    if [[ -z $MAT ]];then
        echo "Warning: Don't know about $NAME"
        continue
    fi

    # So the source file is:
    SRC="$MATERIAL/svg/$MAT.svg"
    if [ ! -f "$SRC" ];then
        echo "Error: Source for $NAME doesn't exist: $SRC"
        exit 1
    fi

    # Replace the icon file with the source file
    cp "$SRC" "$DST" || exit

done
