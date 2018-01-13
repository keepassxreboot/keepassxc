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
find_library(ARGON2_LIBRARIES argon2)
mark_as_advanced(ARGON2_LIBRARIES ARGON2_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Argon2 DEFAULT_MSG ARGON2_LIBRARIES ARGON2_INCLUDE_DIR)