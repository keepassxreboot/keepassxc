#.rst:
# FindReadline
# ------------
#
# Find the readline library.
#
# Usage of this module as follows:
#
#     find_package(Readline [REQUIRED] [QUIET])
#     target_link_libraries(<target> PRIVATE Readline::readline)
#
# Variables used by this module:
#
#   Readline_ROOT              - Root directory to search for readline (e.g., /usr/local or the vcpkg installation path).
#                               If not set, the module will search in standard system paths.
#
# Imported Targets
# ^^^^^^^^^^^^^^^
#
# This module defines the following :prop_tgt:`IMPORTED` target:
#
# ``Readline::readline``
#   The readline library, if found.
#
# Result Variables
# ^^^^^^^^^^^^^^
#
# ``Readline_FOUND``           - True if readline is found.
# ``Readline_INCLUDE_DIRS``    - The readline include directories.
# ``Readline_LIBRARIES``      - The readline libraries.

# --- Step 1: Try pkg-config first ---
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(Readline IMPORTED_TARGET readline)
    if(Readline_FOUND)
        # pkg-config found readline
        add_library(Readline::readline ALIAS PkgConfig::Readline)
        return()
    endif()
endif()

# --- Step 2: Manual search if pkg-config fails ---
# Uses Readline_ROOT if defined (CMake convention: <PackageName>_ROOT)
set(_Readline_SEARCH_PATHS)
if(Readline_ROOT)
    list(APPEND _Readline_SEARCH_PATHS "${Readline_ROOT}")
endif()

find_path(Readline_INCLUDE_DIR
        NAMES readline/readline.h
        PATHS ${_Readline_SEARCH_PATHS}
        PATH_SUFFIXES include
        DOC "readline include directory"
)

find_library(Readline_LIBRARY
        NAMES readline
        PATHS ${_Readline_SEARCH_PATHS}
        PATH_SUFFIXES lib lib64
        DOC "readline library"
)

# Managing results with FindPackageHandleStandardArgs
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Readline
        DEFAULT_MSG Readline_INCLUDE_DIR Readline_LIBRARY
)

if(Readline_FOUND)
    set(Readline_INCLUDE_DIRS ${Readline_INCLUDE_DIR})
    set(Readline_LIBRARIES ${Readline_LIBRARY})

    if(NOT TARGET Readline::readline)
        add_library(Readline::readline UNKNOWN IMPORTED)
        set_target_properties(Readline::readline PROPERTIES
                IMPORTED_LOCATION "${Readline_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${Readline_INCLUDE_DIRS}"
        )
    endif()
endif()

# Hide internal variables
mark_as_advanced(
        Readline_INCLUDE_DIR
        Readline_LIBRARY
        Readline_ROOT
)
