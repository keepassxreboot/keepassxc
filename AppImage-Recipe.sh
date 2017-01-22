#!/usr/bin/env bash
# 
# KeePassXC AppImage Recipe
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

if [ "$1" == "" ] || [ "$2" == "" ]; then
    echo "Usage: $(basename $0) APP_NAME RELEASE_VERSION" >&2
    exit 1
fi

if [ -f CHANGELOG ]; then
    echo "This recipe must not be run from the sources root." >&2
    exit 1
fi

if [ ! -d ../bin-release ]; then
    echo "../bin-release does not exist." >&2
    exit 1
fi

APP="$1"
LOWERAPP="$(echo "$APP" | tr '[:upper:]' '[:lower:]')"
VERSION="$2"

mkdir -p $APP.AppDir
wget -q https://github.com/probonopd/AppImages/raw/master/functions.sh -O ./functions.sh
. ./functions.sh

cd $APP.AppDir
cp -a ../../bin-release/* .
cp -a ./usr/local/* ./usr
rm -R ./usr/local
patch_strings_in_file /usr/local ./
patch_strings_in_file /usr ./

get_apprun
copy_deps
delete_blacklisted

get_desktop
get_icon
get_desktopintegration $LOWERAPP

GLIBC_NEEDED=$(glibc_needed)

cd ..

generate_appimage

mv ../out/*.AppImage ..
rmdir ../out > /dev/null 2>&1
