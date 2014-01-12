#  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

find_path(GCRYPT_INCLUDE_DIR gcrypt.h)

find_library(GCRYPT_LIBRARIES gcrypt)

mark_as_advanced(GCRYPT_LIBRARIES GCRYPT_INCLUDE_DIR)

if(GCRYPT_INCLUDE_DIR AND EXISTS "${GCRYPT_INCLUDE_DIR}/gcrypt.h")
    file(STRINGS "${GCRYPT_INCLUDE_DIR}/gcrypt.h" GCRYPT_H REGEX "^#define GCRYPT_VERSION \"[^\"]*\"$")
    string(REGEX REPLACE "^.*GCRYPT_VERSION \"([0-9]+).*$" "\\1" GCRYPT_VERSION_MAJOR "${GCRYPT_H}")
    string(REGEX REPLACE "^.*GCRYPT_VERSION \"[0-9]+\\.([0-9]+).*$" "\\1" GCRYPT_VERSION_MINOR  "${GCRYPT_H}")
    string(REGEX REPLACE "^.*GCRYPT_VERSION \"[0-9]+\\.[0-9]+\\.([0-9]+).*$" "\\1" GCRYPT_VERSION_PATCH "${GCRYPT_H}")
    set(GCRYPT_VERSION_STRING "${GCRYPT_VERSION_MAJOR}.${GCRYPT_VERSION_MINOR}.${GCRYPT_VERSION_PATCH}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gcrypt DEFAULT_MSG GCRYPT_LIBRARIES GCRYPT_INCLUDE_DIR)
