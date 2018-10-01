#  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

find_path(QUAZIP_INCLUDE_DIR quazip5/quazip.h)
find_library(QUAZIP_LIBRARIES quazip5)

mark_as_advanced(QUAZIP_LIBRARIES QUAZIP_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
include_directories(${QUAZIP_INCLUDE_DIR})
find_package_handle_standard_args(QuaZip DEFAULT_MSG QUAZIP_LIBRARIES QUAZIP_INCLUDE_DIR)
