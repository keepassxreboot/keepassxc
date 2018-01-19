#  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

cmake_minimum_required(VERSION 3.1.0)

project(KeePassXC)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo Debug DebugFull Profile MinSizeRel."
      FORCE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Support Visual Studio Code
include(CMakeToolsHelpers OPTIONAL)

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

option(WITH_TESTS "Enable building of unit tests" ON)
option(WITH_GUI_TESTS "Enable building of GUI tests" OFF)
option(WITH_DEV_BUILD "Use only for development. Disables/warns about deprecated methods." OFF)
option(WITH_ASAN "Enable address sanitizer checks (Linux / macOS only)" OFF)
option(WITH_COVERAGE "Use to build with coverage tests (GCC only)." OFF)
option(WITH_APP_BUNDLE "Enable Application Bundle for macOS" ON)

option(WITH_XC_AUTOTYPE "Include Auto-Type." ON)
option(WITH_XC_NETWORKING "Include networking code (e.g. for downlading website icons)." OFF)
option(WITH_XC_BROWSER "Include browser integration with keepassxc-browser." OFF)
option(WITH_XC_HTTP "Include KeePassHTTP-compatible browser integration (deprecated, implies WITH_NETWORKING)." OFF)
option(WITH_XC_YUBIKEY "Include YubiKey support." OFF)
option(WITH_XC_SSHAGENT "Include SSH agent support." OFF)

if(WITH_XC_HTTP)
  message(WARNING "KeePassHTTP support has been deprecated and will be removed in a future version. Please use WITH_XC_BROWSER instead!\n"
          "For enabling / disabling network access code, WITH_XC_HTTP has been replaced by WITH_XC_NETWORKING.")
  set(WITH_XC_NETWORKING ON CACHE BOOL "Include networking code (e.g. for downlading website icons)." FORCE)
endif()

# Process ui files automatically from source files
set(CMAKE_AUTOUIC ON)

set(KEEPASSXC_VERSION_MAJOR "2")
set(KEEPASSXC_VERSION_MINOR "2")
set(KEEPASSXC_VERSION_PATCH "4")
set(KEEPASSXC_VERSION "${KEEPASSXC_VERSION_MAJOR}.${KEEPASSXC_VERSION_MINOR}.${KEEPASSXC_VERSION_PATCH}")

# Distribution info
set(KEEPASSXC_DIST True)
set(KEEPASSXC_DIST_TYPE "Other" CACHE STRING "KeePassXC Distribution type")
set_property(CACHE KEEPASSXC_DIST_TYPE PROPERTY STRINGS Snap AppImage Other)
if(KEEPASSXC_DIST_TYPE STREQUAL "Snap")
  set(KEEPASSXC_DIST_SNAP True)
elseif(KEEPASSXC_DIST_TYPE STREQUAL "AppImage")
  set(KEEPASSXC_DIST_APPIMAGE True)
elseif(KEEPASSXC_DIST_TYPE STREQUAL "Other")
  unset(KEEPASSXC_DIST)
endif()

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

add_definitions(-DQT_NO_EXCEPTIONS -DQT_STRICT_ITERATORS -DQT_NO_CAST_TO_ASCII)

if(WITH_APP_BUNDLE)
  add_definitions(-DWITH_APP_BUNDLE)
endif()

add_gcc_compiler_flags("-fno-common")
add_gcc_compiler_flags("-Wall -Werror -Wextra -Wundef -Wpointer-arith -Wno-long-long")
add_gcc_compiler_flags("-Wformat=2 -Wmissing-format-attribute")
add_gcc_compiler_flags("-fvisibility=hidden")
add_gcc_compiler_cxxflags("-fvisibility-inlines-hidden")

if((CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.8.999) OR CMAKE_COMPILER_IS_CLANGXX)
    add_gcc_compiler_flags("-fstack-protector-strong")
else()
    add_gcc_compiler_flags("-fstack-protector --param=ssp-buffer-size=4")
endif()

add_gcc_compiler_cxxflags("-fno-exceptions -fno-rtti")
add_gcc_compiler_cxxflags("-Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual")
add_gcc_compiler_cflags("-Wchar-subscripts -Wwrite-strings")

if(WITH_ASAN)
  if(NOT (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR APPLE))
      message(FATAL_ERROR "WITH_ASAN is only supported on Linux / macOS at the moment.")
  endif()

  add_gcc_compiler_flags("-fsanitize=address -DWITH_ASAN")

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(NOT (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9))
      add_gcc_compiler_flags("-fsanitize=leak -DWITH_LSAN")
    endif()
  endif()

endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
if (CMAKE_BUILD_TYPE_LOWER MATCHES "(release|relwithdebinfo|minsizerel)")
  add_gcc_compiler_flags("-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2")
endif()

check_c_compiler_flag("-Werror=format-security -Werror=implicit-function-declaration" WERROR_C_AVAILABLE)
check_cxx_compiler_flag("-Werror=format-security" WERROR_CXX_AVAILABLE)
if(WERROR_C_AVAILABLE AND WERROR_CXX_AVAILABLE)
  add_gcc_compiler_flags("-Werror=format-security")
  add_gcc_compiler_cflags("-Werror=implicit-function-declaration")
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wcast-align")

  if(WITH_COVERAGE)
    # Include code coverage, use with -DCMAKE_BUILD_TYPE=Coverage
    include(CodeCoverage)
    setup_target_for_coverage(kp_coverage "make test" coverage)
  endif()
endif()

if(CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcast-align")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  if (CMAKE_COMPILER_IS_CLANGXX)
      add_gcc_compiler_flags("-Qunused-arguments")
  endif()
  add_gcc_compiler_flags("-pie -fPIE")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-add-needed -Wl,--as-needed -Wl,--no-undefined")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,relro,-z,now")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-add-needed -Wl,--as-needed")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-z,relro,-z,now")
endif()

add_gcc_compiler_cxxflags("-std=c++11")

if(APPLE)
  add_gcc_compiler_cxxflags("-stdlib=libc++")
endif()

add_gcc_compiler_cflags("-ansi")

if(WITH_DEV_BUILD)
  add_definitions(-DQT_DEPRECATED_WARNINGS -DGCRYPT_NO_DEPRECATED)
endif()

if(MINGW)
  set(CMAKE_RC_COMPILER_INIT windres)
  enable_language(RC)
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
  if(NOT (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo"))
    # Enable DEP and ASLR
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--nxcompat -Wl,--dynamicbase")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--nxcompat -Wl,--dynamicbase")
  endif()
endif()

if(APPLE AND WITH_APP_BUNDLE OR MINGW)
  set(PROGNAME KeePassXC)
else()
  set(PROGNAME keepassxc)
endif()

if(APPLE AND WITH_APP_BUNDLE AND "${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local")
  set(CMAKE_INSTALL_PREFIX "/Applications")
  set(CMAKE_INSTALL_MANDIR "/usr/local/share/man")
endif()

if(MINGW)
  set(CLI_INSTALL_DIR    ".")
  set(PROXY_INSTALL_DIR  ".")
  set(BIN_INSTALL_DIR    ".")
  set(PLUGIN_INSTALL_DIR ".")
  set(DATA_INSTALL_DIR   "share")
elseif(APPLE AND WITH_APP_BUNDLE)
  set(CLI_INSTALL_DIR    "/usr/local/bin")
  set(PROXY_INSTALL_DIR  "/usr/local/bin")
  set(BIN_INSTALL_DIR    ".")
  set(PLUGIN_INSTALL_DIR "${PROGNAME}.app/Contents/PlugIns")
  set(DATA_INSTALL_DIR   "${PROGNAME}.app/Contents/Resources")
else()
  include(GNUInstallDirs)

  set(CLI_INSTALL_DIR    "${CMAKE_INSTALL_BINDIR}")
  set(PROXY_INSTALL_DIR  "${CMAKE_INSTALL_BINDIR}")
  set(BIN_INSTALL_DIR    "${CMAKE_INSTALL_BINDIR}")
  set(PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/keepassxc")
  set(DATA_INSTALL_DIR   "${CMAKE_INSTALL_DATADIR}/keepassxc")
endif()

if(WITH_TESTS)
  enable_testing()
endif(WITH_TESTS)

if(UNIX AND NOT APPLE)
  find_package(Qt5 COMPONENTS Core Network Concurrent Widgets Test LinguistTools DBus REQUIRED)
elseif(APPLE)
  find_package(Qt5 COMPONENTS Core Network Concurrent Widgets Test LinguistTools REQUIRED
    HINTS /usr/local/Cellar/qt/*/lib/cmake ENV PATH
  )
  find_package(Qt5 COMPONENTS MacExtras
    HINTS /usr/local/Cellar/qt/*/lib/cmake ENV PATH
  )
else()
  find_package(Qt5 COMPONENTS Core Network Concurrent Widgets Test LinguistTools REQUIRED)
endif()

if(Qt5Core_VERSION VERSION_LESS "5.2.0")
  message(FATAL_ERROR "Qt version 5.2.0 or higher is required")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(APPLE)
  set(CMAKE_MACOSX_RPATH TRUE)
  find_program(MACDEPLOYQT_EXE macdeployqt HINTS /usr/local/opt/qt5/bin ENV PATH)
  if(NOT MACDEPLOYQT_EXE)
    message(FATAL_ERROR "macdeployqt is required to build in macOS")
  else()
    message(STATUS "Using macdeployqt: ${MACDEPLOYQT_EXE}")
  endif()
endif()

# Debian sets the the build type to None for package builds.
# Make sure we don't enable asserts there.
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_NONE QT_NO_DEBUG)

find_package(LibGPGError REQUIRED)
find_package(Gcrypt 1.7.0 REQUIRED)
find_package(Argon2 REQUIRED)

find_package(ZLIB REQUIRED)

set(CMAKE_REQUIRED_INCLUDES ${ZLIB_INCLUDE_DIR})

if(ZLIB_VERSION_STRING VERSION_LESS "1.2.0")
  message(FATAL_ERROR "zlib 1.2.0 or higher is required to use the gzip format")
endif()

# Optional
if(WITH_XC_YUBIKEY)
  find_package(YubiKey REQUIRED)

  include_directories(SYSTEM ${YUBIKEY_INCLUDE_DIRS})
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

include(FeatureSummary)

add_subdirectory(src)
add_subdirectory(share)
if(WITH_TESTS)
  add_subdirectory(tests)
endif(WITH_TESTS)

if(PRINT_SUMMARY)
  # This will print ENABLED, REQUIRED and DISABLED
  feature_summary(WHAT ALL)
else()
  # This will only print ENABLED and DISABLED feature
  feature_summary(WHAT ENABLED_FEATURES DESCRIPTION "Enabled features:")
  feature_summary(WHAT DISABLED_FEATURES DESCRIPTION "Disabled features:")
endif()
