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

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo Debug Debugfull Profile MinSizeRel."
      FORCE)
endif()

project(KeePassX)

cmake_minimum_required(VERSION 2.6.4)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

option(WITH_TESTS "Enable building of unit tests" ON)
option(WITH_GUI_TESTS "Enable building of GUI tests" OFF)
option(WITH_LTO "Enable Link Time Optimization (LTO)" OFF)
option(WITH_CXX11 "Build with the C++ 11 standard" ON)

set(KEEPASSX_VERSION "2.0 alpha 6")
set(KEEPASSX_VERSION_NUM "1.9.85")

if("${CMAKE_C_COMPILER}" MATCHES "clang$" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_COMPILER_IS_CLANG 1)
endif()

if("${CMAKE_CXX_COMPILER}" MATCHES "clang(\\+\\+)?$" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

macro(add_gcc_compiler_cxxflags FLAGS)
  if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS}")
  endif()
endmacro(add_gcc_compiler_cxxflags)

macro(add_gcc_compiler_cflags FLAGS)
  if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS}")
  endif()
endmacro(add_gcc_compiler_cflags)

macro(add_gcc_compiler_flags FLAGS)
  add_gcc_compiler_cxxflags("${FLAGS}")
  add_gcc_compiler_cflags("${FLAGS}")
endmacro(add_gcc_compiler_flags)

add_definitions(-DQT_NO_KEYWORDS -DQT_NO_EXCEPTIONS -DQT_NO_STL -DQT_STRICT_ITERATORS -DQT_NO_CAST_TO_ASCII)

add_gcc_compiler_flags("-fno-common -fstack-protector --param=ssp-buffer-size=4")
add_gcc_compiler_flags("-Wall -Wextra -Wundef -Wpointer-arith -Wno-long-long")
add_gcc_compiler_flags("-Wformat=2 -Wmissing-format-attribute")
add_gcc_compiler_flags("-fvisibility=hidden")
add_gcc_compiler_cxxflags("-fvisibility-inlines-hidden")

add_gcc_compiler_cxxflags("-fno-exceptions -fno-rtti")
add_gcc_compiler_cxxflags("-Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual")
add_gcc_compiler_cflags("-Wchar-subscripts -Wwrite-strings")

string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
if (CMAKE_BUILD_TYPE_LOWER MATCHES (release|relwithdebinfo|minsizerel))
  add_gcc_compiler_flags("-D_FORTIFY_SOURCE=2")
endif()

check_c_compiler_flag("-Werror=format-security -Werror=implicit-function-declaration" WERROR_C_AVAILABLE)
check_cxx_compiler_flag("-Werror=format-security" WERROR_CXX_AVAILABLE)
if(WERROR_C_AVAILABLE AND WERROR_CXX_AVAILABLE)
  add_gcc_compiler_flags("-Werror=format-security")
  add_gcc_compiler_cflags("-Werror=implicit-function-declaration")
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wcast-align")
endif()

if(CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcast-align")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-add-needed -Wl,--as-needed -Wl,--no-undefined")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,relro")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-add-needed -Wl,--as-needed")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-z,relro")
endif()

if(WITH_LTO)
  if(CMAKE_COMPILER_IS_GNUCC AND CMAKE_COMPILER_IS_GNUCXX)
    check_cxx_compiler_flag("-flto -fuse-linker-plugin" LTO_AVAILABLE)

    if(LTO_AVAILABLE)
      add_gcc_compiler_flags("-flto -fuse-linker-plugin")
    else()
      message(FATAL_ERROR "This version of gcc doesn't support LTO")
    endif(LTO_AVAILABLE)
  else()
    message(FATAL_ERROR "LTO is only supported with gcc")
  endif()
endif()

if (WITH_CXX11)
  add_gcc_compiler_cxxflags("-std=c++0x")
  add_gcc_compiler_cflags("-ansi")
else()
  add_gcc_compiler_flags("-ansi")
endif()

if(MINGW)
  set(CMAKE_RC_COMPILER_INIT windres)
  enable_language(RC)
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
endif()

if(APPLE OR MINGW)
  set(PROGNAME KeePassX)
else()
  set(PROGNAME keepassx)
endif()

if(APPLE AND "${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local")
  set(CMAKE_INSTALL_PREFIX "/Applications")
endif()

if(MINGW)
  set(BIN_INSTALL_DIR    ".")
  set(PLUGIN_INSTALL_DIR ".")
  set(DATA_INSTALL_DIR   "share")
elseif(APPLE)
  set(BIN_INSTALL_DIR    ".")
  set(PLUGIN_INSTALL_DIR ".")
  set(DATA_INSTALL_DIR   "${PROGNAME}.app/Contents/Resources")
else()
  include(GNUInstallDirs)

  set(BIN_INSTALL_DIR    "${CMAKE_INSTALL_BINDIR}")
  set(PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/keepassx")
  set(DATA_INSTALL_DIR   "${CMAKE_INSTALL_DATAROOTDIR}/keepassx")
endif()

if(WITH_TESTS)
  enable_testing()
endif(WITH_TESTS)

set(QT_REQUIRED_MODULES QtCore QtGui QtTest)
if(UNIX AND NOT APPLE)
  set(QT_REQUIRED_MODULES ${QT_REQUIRED_MODULES} QtDBus)
endif()

find_package(Qt4 4.6.0 REQUIRED ${QT_REQUIRED_MODULES})
include(${QT_USE_FILE})

find_package(Gcrypt REQUIRED)
if(NOT (${GCRYPT_VERSION_STRING} VERSION_LESS "1.6.0"))
  message(STATUS "Gcrypt ${GCRYPT_VERSION_STRING} supports the SALSA20 cipher")
  set(GCRYPT_HAS_SALSA20 1)
endif()

find_package(ZLIB REQUIRED)

check_cxx_source_compiles("
  #include <zlib.h>

  #if !defined(ZLIB_VERNUM) || (ZLIB_VERNUM < 0x1200)
      #error zlib 1.2.x or higher is required to use the gzip format
  #endif

  int main() { return 0; }" ZLIB_SUPPORTS_GZIP)

if(NOT ZLIB_SUPPORTS_GZIP)
  message(FATAL_ERROR "zlib 1.2.x or higher is required to use the gzip format")
endif()

if(UNIX)
  check_cxx_source_compiles("#include <sys/prctl.h>
    int main() { prctl(PR_SET_DUMPABLE, 0); return 0; }"
    HAVE_PR_SET_DUMPABLE)

  check_cxx_source_compiles("#include <sys/resource.h>
    int main() {
      struct rlimit limit;
      limit.rlim_cur = 0;
      limit.rlim_max = 0;
      setrlimit(RLIMIT_CORE, &limit);
      return 0;
    }" HAVE_RLIMIT_CORE)

  if(APPLE)
    check_cxx_source_compiles("#include <sys/types.h>
      #include <sys/ptrace.h>
      int main() { ptrace(PT_DENY_ATTACH, 0, 0, 0); return 0; }"
      HAVE_PT_DENY_ATTACH)
  endif()
endif()

include_directories(SYSTEM ${GCRYPT_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})

if(NOT (${CMAKE_VERSION} VERSION_LESS 2.8.3))
  set(PRINT_SUMMARY ON)
  include(FeatureSummary)
endif()

add_subdirectory(src)
add_subdirectory(share)
add_subdirectory(utils)
if(WITH_TESTS)
  add_subdirectory(tests)
endif(WITH_TESTS)

if(PRINT_SUMMARY)
  feature_summary(WHAT ALL)
endif()
