#    /cpp-readline/cmake/modules/FindReadline.cmake
#
#    @author zmij
#    @date May 17, 2016

# - Try to find readline include dirs and libraries
#
# Usage of this module as follows:
#
#     find_package(Readline)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  Readline_ROOT_DIR         Set this variable to the root installation of
#                            readline if the module has problems finding the
#                            proper installation path.
#
# Variables defined by this module:
#
#  READLINE_FOUND            System has readline, include and lib dirs found
#  Readline_INCLUDE_DIR      The readline include directories.
#  Readline_LIBRARY          The readline library.

include(FindPackageHandleStandardArgs)
if (Readline_INCLUDE_DIR)
    set(READLINE_FOUND TRUE)
else()

    find_path(Readline_ROOT_DIR
        NAMES include/readline/readline.h
        HINTS /usr/local/Cellar/readline/*/
        NO_DEFAULT_PATH
    )

    find_path(Readline_INCLUDE_DIR
        NAMES readline/readline.h
        HINTS ${Readline_ROOT_DIR}/include
    )

    find_library(Readline_LIBRARY
        NAMES readline
        HINTS ${Readline_ROOT_DIR}/lib
    )

    if(Readline_INCLUDE_DIR AND Readline_LIBRARY AND Ncurses_LIBRARY)
      set(READLINE_FOUND TRUE)
    else(Readline_INCLUDE_DIR AND Readline_LIBRARY AND Ncurses_LIBRARY)
      find_library(Readline_LIBRARY NAMES readline)
      FIND_PACKAGE_HANDLE_STANDARD_ARGS(Readline DEFAULT_MSG Readline_INCLUDE_DIR Readline_LIBRARY)
      mark_as_advanced(Readline_INCLUDE_DIR Readline_LIBRARY)
    endif(Readline_INCLUDE_DIR AND Readline_LIBRARY AND Ncurses_LIBRARY)

    mark_as_advanced(
        Readline_ROOT_DIR
        Readline_INCLUDE_DIR
        Readline_LIBRARY
    )

endif()
