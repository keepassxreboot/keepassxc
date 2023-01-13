# Copyright (c) 2018 Ribose Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

#.rst:
# FindBotan3
# -----------
#
# Find the botan-3 library.
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines :prop_tgt:`IMPORTED` targets:
#
# ``Botan3::Botan3``
#   The botan-3 library, if found.
#
# Result variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ::
#
#   BOTAN3_FOUND          - true if the headers and library were found
#   BOTAN3_INCLUDE_DIRS   - where to find headers
#   BOTAN3_LIBRARIES      - list of libraries to link
#   BOTAN3_VERSION        - library version that was found, if any

# find the headers
find_path(BOTAN3_INCLUDE_DIR
  NAMES botan/version.h
  PATH_SUFFIXES botan-3
)

# find the library
find_library(BOTAN3_LIBRARY NAMES botan-3 libbotan-3 botan)

# determine the version
if(BOTAN3_INCLUDE_DIR AND EXISTS "${BOTAN3_INCLUDE_DIR}/botan/build.h")
    file(STRINGS "${BOTAN3_INCLUDE_DIR}/botan/build.h" botan3_version_str
      REGEX "^#define[\t ]+(BOTAN_VERSION_[A-Z]+)[\t ]+[0-9]+")

    string(REGEX REPLACE ".*#define[\t ]+BOTAN_VERSION_MAJOR[\t ]+([0-9]+).*"
           "\\1" _botan3_version_major "${botan3_version_str}")
    string(REGEX REPLACE ".*#define[\t ]+BOTAN_VERSION_MINOR[\t ]+([0-9]+).*"
           "\\1" _botan3_version_minor "${botan3_version_str}")
    string(REGEX REPLACE ".*#define[\t ]+BOTAN_VERSION_PATCH[\t ]+([0-9]+).*"
           "\\1" _botan3_version_patch "${botan3_version_str}")
    set(BOTAN3_VERSION "${_botan3_version_major}.${_botan3_version_minor}.${_botan3_version_patch}"
                       CACHE INTERNAL "The version of Botan which was detected")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Botan3
  REQUIRED_VARS BOTAN3_LIBRARY BOTAN3_INCLUDE_DIR
  VERSION_VAR BOTAN3_VERSION
)

if(BOTAN3_FOUND)
  set(BOTAN3_INCLUDE_DIRS ${BOTAN3_INCLUDE_DIR} ${PC_BOTAN3_INCLUDE_DIRS})
  set(BOTAN3_LIBRARIES ${BOTAN3_LIBRARY})
endif()

if(BOTAN3_FOUND AND NOT TARGET Botan3::Botan3)
  # create the new library target
  add_library(Botan3::Botan3 UNKNOWN IMPORTED)
  # set the required include dirs for the target
  if(BOTAN3_INCLUDE_DIRS)
    set_target_properties(Botan3::Botan3
      PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${BOTAN3_INCLUDE_DIRS}"
    )
  endif()
  # set the required libraries for the target
  if(EXISTS "${BOTAN3_LIBRARY}")
    set_target_properties(Botan3::Botan3
      PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${BOTAN3_LIBRARY}"
    )
  endif()
endif()

mark_as_advanced(BOTAN3_INCLUDE_DIR BOTAN3_LIBRARY)
