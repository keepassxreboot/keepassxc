#  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 2 or (at your option)
#  version 3 of the License.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

set(EXCLUDED_DIRS
    # third-party directories
    zxcvbn/
    streams/QtIOCompressor
    # objective-c directories
    autotype/mac
)

set(EXCLUDED_FILES
    # third-party files
    streams/qtiocompressor.cpp
    streams/qtiocompressor.h
    gui/KMessageWidget.h
    gui/KMessageWidget.cpp
    gui/MainWindowAdaptor.h
    gui/MainWindowAdaptor.cpp
    sshagent/bcrypt_pbkdf.cpp
    sshagent/blf.h
    sshagent/blowfish.c
    tests/modeltest.cpp
    tests/modeltest.h
    # objective-c files
    core/ScreenLockListenerMac.h
    core/ScreenLockListenerMac.cpp
)

file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.h)
foreach (SOURCE_FILE ${ALL_SOURCE_FILES})
    foreach (EXCLUDED_DIR ${EXCLUDED_DIRS})
        string(FIND ${SOURCE_FILE} ${EXCLUDED_DIR} SOURCE_FILE_EXCLUDED)
        if (NOT ${SOURCE_FILE_EXCLUDED} EQUAL -1)
            list(REMOVE_ITEM ALL_SOURCE_FILES ${SOURCE_FILE})
        endif ()
    endforeach ()
    foreach (EXCLUDED_FILE ${EXCLUDED_FILES})
        if (${SOURCE_FILE} MATCHES ".*${EXCLUDED_FILE}$")
            list(REMOVE_ITEM ALL_SOURCE_FILES ${SOURCE_FILE})
        endif ()
    endforeach ()
endforeach ()

add_custom_target(
        format
        COMMAND echo ${ALL_SOURCE_FILES} | xargs clang-format -style=file -i
)
