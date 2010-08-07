#  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

project(keepassx)

cmake_minimum_required(VERSION 2.6.0)

set( keepassx_VERSION "0.9.0" )

add_Definitions(-D'KEEPASSX_VERSION="${keepassx_VERSION}"')

if( APPLE OR MINGW )
    set( PROGNAME KeePassX )
else( APPLE OR MINGW )
    set( PROGNAME keepassx )
endif( APPLE OR MINGW )

set(QT_MIN_VERSION "4.3.0")
set(QT_USE_QTXML TRUE)
set(QT_USE_QTMAIN TRUE)
find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})

add_subdirectory(src)
