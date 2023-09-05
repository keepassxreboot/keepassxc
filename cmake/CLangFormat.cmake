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
        src/thirdparty
        src/zxcvbn
        # objective-c directories
        src/quickunlock/touchid
        src/autotype/mac
        src/gui/osutils/macutils)

set(EXCLUDED_FILES
        # third-party files
        src/streams/qtiocompressor.\\*
        src/gui/KMessageWidget.\\*
        src/gui/MainWindowAdaptor.\\*
        src/gui/tag/TagsEdit.\\*
        tests/modeltest.\\*
        # objective-c files
        src/core/ScreenLockListenerMac.\\*)

set(FIND_EXCLUDE_DIR_EXPR "")
foreach(EXCLUDE ${EXCLUDED_DIRS})
    list(APPEND FIND_EXCLUDE_DIR_EXPR -o -path "${EXCLUDE}" -prune)
endforeach()

set(FIND_EXCLUDE_FILE_EXPR "")
foreach(EXCLUDE ${EXCLUDED_FILES})
    if(FIND_EXCLUDE_FILE_EXPR)
        list(APPEND FIND_EXCLUDE_FILE_EXPR -o)
    endif()
    list(APPEND FIND_EXCLUDE_FILE_EXPR -path "${EXCLUDE}")
endforeach()
if(FIND_EXCLUDE_FILE_EXPR)
    set(FIND_EXCLUDE_FILE_EXPR -a -not "\\(" ${FIND_EXCLUDE_FILE_EXPR} "\\)")
endif()

add_custom_target(format)

add_custom_command(
        TARGET format
        PRE_BUILD
        COMMAND find src tests "\\(" -name "\\*.h" -o -name "\\*.cpp" ${FIND_EXCLUDE_DIR_EXPR} "\\)"
            ${FIND_EXCLUDE_FILE_EXPR} -type f -print0 | xargs -0 -P0 -n10 clang-format -style=file -i

        COMMENT "Formatting source files..."
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
