#!/usr/bin/env bash
#
# Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

BASEDIR="$(dirname $0)"

PUSH=true
PULL=true
UPDATE=true

if [ "$1" == "push" ]; then
    PULL=false
elif [ "$1" == "pull" ]; then
    PUSH=false
    UPDATE=false
elif [ "$1" == "update" ]; then
    PUSH=false
    PULL=false
elif [ "$1" != "" ]; then
    echo "Unknown command '${1}'"
    echo "Usage: $(basename $0) [update|pull|push] [additional tx options]"
    exit 1
fi

shift

cd "${BASEDIR}/../.."

if $UPDATE; then
    echo "Updating source files..."

    LUPDATE=lupdate-qt5
    command -v $LUPDATE > /dev/null
    if [ $? -ne 0 ]; then
        LUPDATE=lupdate
    fi
    $LUPDATE -no-ui-lines -disable-heuristic similartext -locations none -no-obsolete src -ts share/translations/keepassx_en.ts
    echo
fi

if $PUSH; then
    echo "Pushing source files to Transifex..."
    tx push -s $@
    echo
fi

if $PULL; then
    echo "Removing stale translations..."
    mv share/translations/keepassx_en.ts share/translations/keepassx_en.ts.bak
    rm share/translations/*.ts
    mv share/translations/keepassx_en.ts.bak share/translations/keepassx_en.ts

    echo "Pulling translations from Transifex..."
    tx pull -af --minimum-perc=40 $@
    echo
fi
