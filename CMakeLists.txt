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

add_definitions(-DQT_NO_KEYWORDS -DQT_NO_EXCEPTIONS -DQT_NO_STL -DQT_STRICT_ITERATORS)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ansi -fno-common -fno-exceptions -fno-rtti")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wundef -Wold-style-cast -Wnon-virtual-dtor -Wcast-align")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpointer-arith -Wformat-security -Woverloaded-virtual -Wno-long-long")

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -fno-common")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wundef -Wcast-align -Wchar-subscripts -Wpointer-arith")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wwrite-strings -Wformat-security -Werror-implicit-function-declaration")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-format-attribute -Wno-long-long")

    if( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-add-needed -Wl,--as-needed -Wl,--no-undefined" )
    endif( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
endif(CMAKE_COMPILER_IS_GNUCXX)

if( APPLE OR MINGW )
    set( PROGNAME KeePassX )
else( APPLE OR MINGW )
    set( PROGNAME keepassx )
endif( APPLE OR MINGW )

if( WITH_TESTS )
    enable_testing()
endif( WITH_TESTS )

find_package(Qt4 4.6.0 REQUIRED QtCore QtGui QtTest)
add_definitions(${QT_DEFINITIONS} -DQT_CORE_LIB -DQT_GUI_LIB)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG QT_DEBUG)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_RELEASE QT_NO_DEBUG)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_RELWITHDEBINFO QT_NO_DEBUG)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_MINSIZEREL QT_NO_DEBUG)

find_package(Gcrypt REQUIRED)

find_package(ZLIB REQUIRED)

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
    #include <zlib.h>

    #if !defined(ZLIB_VERNUM) || (ZLIB_VERNUM < 0x1200)
        #error zlib 1.2.x or higher is required to use the gzip format
    #endif

    int main() { return 0; }" ZLIB_SUPPORTS_GZIP)

if(NOT ZLIB_SUPPORTS_GZIP)
    message(FATAL_ERROR "zlib 1.2.x or higher is required to use the gzip format")
endif(NOT ZLIB_SUPPORTS_GZIP)

include_directories(${QT_INCLUDE_DIR} ${GCRYPT_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})

add_subdirectory(src)
add_subdirectory(utils)
if( WITH_TESTS )
    add_subdirectory(tests)
endif( WITH_TESTS )
