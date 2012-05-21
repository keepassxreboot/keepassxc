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

cmake_minimum_required(VERSION 2.6.4)

set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake )

option(WITH_TESTS "Enable building of unit tests" ON)
option(WITH_GUI_TESTS "Enable building of GUI tests" OFF)
option(WITH_LTO "Enable Link Time Optimization (LTO)" OFF)
option(WITH_PIE "Build as Position-independent executable (PIE)" OFF)

set( KEEPASSX_VERSION "2.0 alpha 1" )

if("${CMAKE_C_COMPILER}" MATCHES "clang$" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_COMPILER_IS_CLANG 1)
endif("${CMAKE_C_COMPILER}" MATCHES "clang$" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")

if("${CMAKE_CXX_COMPILER}" MATCHES "clang(\\+\\+)?$" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_COMPILER_IS_CLANGXX 1)
endif("${CMAKE_CXX_COMPILER}" MATCHES "clang(\\+\\+)?$" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

macro(add_gcc_compiler_cxxflags FLAGS)
  if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS}")
  endif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
endmacro(add_gcc_compiler_cxxflags)

macro(add_gcc_compiler_cflags FLAGS)
  if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS}")
  endif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
endmacro(add_gcc_compiler_cflags)

macro(add_gcc_compiler_flags FLAGS)
  add_gcc_compiler_cxxflags("${FLAGS}")
  add_gcc_compiler_cflags("${FLAGS}")
endmacro(add_gcc_compiler_flags)

add_definitions(-DQT_NO_KEYWORDS -DQT_NO_EXCEPTIONS -DQT_NO_STL -DQT_STRICT_ITERATORS -DQT_NO_CAST_TO_ASCII)

add_gcc_compiler_flags("-ansi -fno-common -fstack-protector -D_FORTIFY_SOURCE=2")
add_gcc_compiler_flags("-Wall -Wextra -Wundef -Wpointer-arith -Wno-long-long")
add_gcc_compiler_flags("-Wformat=2 -Werror=format-security -Wmissing-format-attribute")

add_gcc_compiler_cxxflags("-fno-exceptions -fno-rtti")
add_gcc_compiler_cxxflags("-Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual")
add_gcc_compiler_cflags("-Wchar-subscripts -Wwrite-strings -Werror=implicit-function-declaration")

if(CMAKE_COMPILER_IS_CLANGXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mllvm -stack-protector-buffer-size=4")
elseif(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wcast-align --param=ssp-buffer-size=4")
endif(CMAKE_COMPILER_IS_CLANGXX)

if(CMAKE_COMPILER_IS_CLANG)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mllvm -stack-protector-buffer-size=4")
elseif(CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcast-align --param=ssp-buffer-size=4")
endif(CMAKE_COMPILER_IS_CLANG)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-add-needed -Wl,--as-needed -Wl,--no-undefined")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,relro")
endif(CMAKE_SYSTEM_NAME STREQUAL "Linux")

if(WITH_LTO)
  include(CheckCXXCompilerFlag)

  if(CMAKE_COMPILER_IS_GNUCC and CMAKE_COMPILER_IS_GNUCXX)
    check_cxx_compiler_flag("-flto -fuse-linker-plugin" LTO_AVAILABLE)

    if(LTO_AVAILABLE)
      add_gcc_compiler_flags("-flto -fuse-linker-plugin")
    else()
      message(FATAL_ERROR "This version of gcc doesn't support LTO")
    endif(LTO_AVAILABLE)
  else()
    message(FATAL_ERROR "LTO is only supported with gcc")
  endif(CMAKE_COMPILER_IS_GNUCC and CMAKE_COMPILER_IS_GNUCXX)
endif(WITH_LTO)

if(WITH_PIE)
  if((CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG) AND (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX))
    add_gcc_compiler_flags("-fPIE")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE -pie")
  else()
    message(FATAL_ERROR "PIE is only supported with gcc and clang")
  endif((CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG) AND (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX))
endif(WITH_PIE)

if( APPLE OR MINGW )
    set( PROGNAME KeePassX )
else( APPLE OR MINGW )
    set( PROGNAME keepassx )
endif( APPLE OR MINGW )

if(MINGW)
    set(BIN_INSTALL_DIR  "")
    set(DATA_INSTALL_DIR "share")
elseif(APPLE)
    set(BIN_INSTALL_DIR  "")
    set(DATA_INSTALL_DIR "${PROGNAME}.app/Contents/Resources")
else(MINGW)
    set(BIN_INSTALL_DIR  "bin")
    set(DATA_INSTALL_DIR "share/keepassx")
endif( MINGW )

if( WITH_TESTS )
    enable_testing()
endif( WITH_TESTS )

find_package(Qt4 4.6.0 REQUIRED QtCore QtGui QtTest)
add_definitions(${QT_DEFINITIONS} -DQT_CORE_LIB -DQT_GUI_LIB)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG QT_DEBUG)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_RELEASE QT_NO_DEBUG)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_RELWITHDEBINFO QT_NO_DEBUG)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_MINSIZEREL QT_NO_DEBUG)
IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS QT_NO_DEBUG)
ENDIF()

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

include_directories(SYSTEM ${QT_INCLUDE_DIR} ${GCRYPT_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})

add_subdirectory(src)
add_subdirectory(share)
add_subdirectory(utils)
if( WITH_TESTS )
    add_subdirectory(tests)
endif( WITH_TESTS )
