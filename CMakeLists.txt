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

project(KeePassX)

cmake_minimum_required(VERSION 2.6.0)

set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake )

option(WITH_TESTS "Enable building of unit tests" ON)

set( KEEPASSX_VERSION "0.9.0" )

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-DQT_NO_KEYWORDS -DQT_NO_EXCEPTIONS -fno-exceptions -fno-rtti
                    -ansi -Wall -Wextra -Wundef -Wold-style-cast -Wnon-virtual-dtor -Wcast-align
                    -Wpointer-arith -Wformat-security -Woverloaded-virtual -Wno-long-long)
endif(CMAKE_COMPILER_IS_GNUCXX)

if( APPLE OR MINGW )
    set( PROGNAME KeePassX )
else( APPLE OR MINGW )
    set( PROGNAME keepassx )
endif( APPLE OR MINGW )

if( WITH_TESTS )
    enable_testing()
    set(QT_USE_QTTEST TRUE)
endif( WITH_TESTS )

set(QT_MIN_VERSION "4.6.0")
set(QT_USE_QTMAIN TRUE)
find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})

find_package(Automoc4 REQUIRED)

find_package(Libgcrypt REQUIRED)

add_subdirectory(src)
if( WITH_TESTS )
    add_subdirectory(tests)
endif( WITH_TESTS )
