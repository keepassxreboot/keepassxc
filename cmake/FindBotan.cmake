# - Find botan
# Find the botan cryptographic library
#
# This module defines the following variables:
#   BOTAN_FOUND  -  True if library and include directory are found
# If set to TRUE, the following are also defined:
#   BOTAN_INCLUDE_DIRS  -  The directory where to find the header file
#   BOTAN_LIBRARIES  -  Where to find the library files
#
# This file is in the public domain (https://github.com/vistle/vistle/blob/master/cmake/Modules/FindBOTAN.cmake)

include(FindPackageHandleStandardArgs)

set(BOTAN_VERSIONS botan-3 botan-2)
set(BOTAN_NAMES botan-3 botan-2 botan)
set(BOTAN_NAMES_DEBUG botand-3 botand-2 botand botan botan-3)

find_path(
    BOTAN_INCLUDE_DIR
    NAMES botan/build.h
    PATH_SUFFIXES ${BOTAN_VERSIONS}
    DOC "The Botan include directory")
if(BOTAN_INCLUDE_DIR)
    file(READ "${BOTAN_INCLUDE_DIR}/botan/build.h" build)
    string(REGEX MATCH "BOTAN_VERSION_MAJOR ([0-9]*)" _ ${build})
    set(BOTAN_VERSION_MAJOR ${CMAKE_MATCH_1})
    string(REGEX MATCH "BOTAN_VERSION_MINOR ([0-9]*)" _ ${build})
    set(BOTAN_VERSION_MINOR ${CMAKE_MATCH_1})
    string(REGEX MATCH "BOTAN_VERSION_PATCH ([0-9]*)" _ ${build})
    set(BOTAN_VERSION_PATCH ${CMAKE_MATCH_1})
    set(BOTAN_VERSION "${BOTAN_VERSION_MAJOR}.${BOTAN_VERSION_MINOR}.${BOTAN_VERSION_PATCH}")
endif()

find_library(
    BOTAN_LIBRARY
    NAMES ${BOTAN_NAMES}
    PATH_SUFFIXES release/lib lib
    DOC "The Botan (release) library")
if(MSVC)
    find_library(
        BOTAN_LIBRARY_DEBUG
        NAMES ${BOTAN_NAMES_DEBUG}
        PATH_SUFFIXES debug/lib lib
        DOC "The Botan debug library")
    find_package_handle_standard_args(
        Botan
        REQUIRED_VARS BOTAN_LIBRARY BOTAN_LIBRARY_DEBUG BOTAN_INCLUDE_DIR
        VERSION_VAR BOTAN_VERSION)
else()
    find_package_handle_standard_args(
        Botan
        REQUIRED_VARS BOTAN_LIBRARY BOTAN_INCLUDE_DIR
        VERSION_VAR BOTAN_VERSION)
endif()

if(BOTAN_FOUND)
    set(BOTAN_INCLUDE_DIRS ${BOTAN_INCLUDE_DIR})
    if(MSVC)
        set(BOTAN_LIBRARIES optimized ${BOTAN_LIBRARY} debug ${BOTAN_LIBRARY_DEBUG})
    else()
        set(BOTAN_LIBRARIES ${BOTAN_LIBRARY})
    endif()
endif()

mark_as_advanced(BOTAN_INCLUDE_DIR BOTAN_LIBRARY BOTAN_LIBRARY_DEBUG)
