# - Try to find the linenoise library
#
# Usage of this module as follows:
#
#     find_package(Linenoise)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  Linenoise_ROOT_DIR       Set this variable to the root installation of
#                           linenoise if the module has problems finding the
#                           proper installation path.
#
# Variables defined by this module:
#
#  LINENOISE_FOUND          System has linenoise,lib dir found
#  Linenoise_LIBRARY        The linenoise library.

find_path(Linenoise_ROOT_DIR
    NAMES include/linenoise.h
)

find_library(Linenoise_LIBRARY
    NAMES linenoise
    HINTS ${Linenoise_ROOT_DIR}/lib
)

if(Linenoise_LIBRARY)
  set(LINENOISE_FOUND TRUE)
  find_library(Linenoise_LIBRARY NAMES linenoise)
endif(Linenoise_LIBRARY)

mark_as_advanced(Linenoise_LIBRARY Linenoise_ROOT_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Linenoise DEFAULT_MSG Linenoise_LIBRARY Linenoise_ROOT_DIR)
