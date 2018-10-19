#  Copyright (C) 2017 KeePassXC Team
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

find_path(ARGON2_INCLUDE_DIR argon2.h)
if(MINGW)
    # find static library on Windows, and redefine used symbols to
    # avoid definition name conflicts with libsodium
    find_library(ARGON2_SYS_LIBRARIES libargon2.a)
    message(STATUS "Patching libargon2...\n")
    execute_process(COMMAND objcopy
            --redefine-sym argon2_hash=libargon2_argon2_hash
            --redefine-sym _argon2_hash=_libargon2_argon2_hash
            --redefine-sym argon2_error_message=libargon2_argon2_error_message
            --redefine-sym _argon2_error_message=_libargon2_argon2_error_message
            ${ARGON2_SYS_LIBRARIES} ${CMAKE_BINARY_DIR}/libargon2_patched.a
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    find_library(ARGON2_LIBRARIES libargon2_patched.a PATHS ${CMAKE_BINARY_DIR} NO_DEFAULT_PATH)
else()
    find_library(ARGON2_LIBRARIES argon2)
endif()
mark_as_advanced(ARGON2_LIBRARIES ARGON2_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Argon2 DEFAULT_MSG ARGON2_LIBRARIES ARGON2_INCLUDE_DIR)
