#  Copyright (C) 2014 Kyle Manna <kyle@kylemanna.com>
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

find_path(YUBIKEY_CORE_INCLUDE_DIR yubikey.h)
find_path(YUBIKEY_PERS_INCLUDE_DIR ykcore.h PATH_SUFFIXES ykpers-1)
set(YUBIKEY_INCLUDE_DIRS ${YUBIKEY_CORE_INCLUDE_DIR} ${YUBIKEY_PERS_INCLUDE_DIR})

find_library(YUBIKEY_CORE_LIBRARY yubikey)
find_library(YUBIKEY_PERS_LIBRARY ykpers-1)
set(YUBIKEY_LIBRARIES ${YUBIKEY_CORE_LIBRARY} ${YUBIKEY_PERS_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(YubiKey DEFAULT_MSG YUBIKEY_LIBRARIES YUBIKEY_INCLUDE_DIRS)

# TODO: Is mark_as_advanced() necessary? It's used in many examples with
#       little explanation.  Disable for now in favor of simplicity.
#mark_as_advanced(YUBIKEY_LIBRARIES YUBIKEY_INCLUDE_DIRS)
