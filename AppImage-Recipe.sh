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

# exit with error if any sub command fails
set -e

APP="$1"
LOWERAPP="$(echo "$APP" | tr '[:upper:]' '[:lower:]')"
VERSION="$2"
export ARCH=x86_64

mkdir -p $APP.AppDir
wget -q https://github.com/AppImage/AppImages/raw/master/functions.sh -O ./functions.sh 2>&1
. ./functions.sh

LIB_DIR=./usr/lib
if [ -d ./usr/lib/x86_64-linux-gnu ]; then
    LIB_DIR=./usr/lib/x86_64-linux-gnu
elif [ -d ./usr/lib/i386-linux-gnu ]; then
    LIB_DIR=./usr/lib/i386-linux-gnu
elif [ -d ./usr/lib64 ]; then
    LIB_DIR=./usr/lib64
fi

cd $APP.AppDir
cp -a ../../bin-release/* .
cp -a ./usr/local/* ./usr
rm -R ./usr/local
rmdir ./opt 2> /dev/null

# bundle Qt platform plugins and themes
QXCB_PLUGIN="$(find /usr/lib* -name 'libqxcb.so' 2> /dev/null)"
if [ "$QXCB_PLUGIN" == "" ]; then
    QXCB_PLUGIN="$(find /opt/qt*/plugins -name 'libqxcb.so' 2> /dev/null)"
fi
QT_PLUGIN_PATH="$(dirname $(dirname $QXCB_PLUGIN))"
mkdir -p ".${QT_PLUGIN_PATH}/platforms"
cp "$QXCB_PLUGIN" ".${QT_PLUGIN_PATH}/platforms/"

get_apprun 2>&1
copy_deps 2>&1
delete_blacklisted 2>&1

get_desktop 2>&1
get_icon 2>&1
cat << EOF > ./usr/bin/keepassxc_env
#!/usr/bin/env bash
export LD_LIBRARY_PATH="..$(dirname ${QT_PLUGIN_PATH})/lib:\${LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="..${QT_PLUGIN_PATH}:\${KPXC_QT_PLUGIN_PATH}"

# unset XDG_DATA_DIRS to make tray icon work in Ubuntu Unity
# see https://github.com/AppImage/AppImageKit/issues/351
unset XDG_DATA_DIRS

if [ "\${1}" == "cli" ]; then
    shift
    exec keepassxc-cli "\$@"
else
    exec keepassxc "\$@"
fi
EOF
chmod +x ./usr/bin/keepassxc_env
sed -i 's/Exec=keepassxc/Exec=keepassxc_env/' org.${LOWERAPP}.${APP}.desktop
get_desktopintegration "org.${LOWERAPP}.${APP}" 2>&1

cd ..

GLIBC_NEEDED=$(glibc_needed)
NO_GLIBC_VERSION=true

generate_type2_appimage -u "gh-releases-zsync|keepassxreboot|keepassxc|latest|KeePassXC-*-${ARCH}.AppImage.zsync" 2>&1

mv ../out/*.AppImage* ../
rm -rf ../out
