#[=======================================================================[.rst:
FindBotan
---------

Finds the botan cryptographic library

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``Botan::botan``
  The Botan library

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

``Botan_FOUND``
  Boolean indicating whether Botan was found.
``Botan_VERSION``
  The version of the Botan library which was found.
``Botan_LIBRARY``
  The path to the Botan library

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Botan_INCLUDE_DIR``
  The directory containing ``botan/build.h``.
``Botan_LIBRARY_DEBUG``
  The debug version of the Botan library, if present
``Botan_LIBRARY_RELEASE``
  The release version of the Botan library

#]=======================================================================]

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_search_module(PC_Botan QUIET botan-3 botan-2>=2.19.1 libbotan-2>=2.19.1 botan>=2.19.1)
endif()

set(BOTAN_VERSIONS botan-3 botan-2)
set(BOTAN_NAMES botan-3 botan-2 botan)
set(BOTAN_NAMES_DEBUG botand-3 botand-2 botand botan botan-3)

find_path(
    Botan_INCLUDE_DIR
    NAMES botan/build.h
    HINTS ${PC_Botan_INCLUDE_DIRS}
    PATH_SUFFIXES ${BOTAN_VERSIONS}
    DOC "The Botan include directory")

if(Botan_INCLUDE_DIR)
    file(READ "${Botan_INCLUDE_DIR}/botan/build.h" build)
    string(REGEX MATCH "BOTAN_VERSION_MAJOR ([0-9]*)" _ ${build})
    set(Botan_VERSION_MAJOR ${CMAKE_MATCH_1})
    string(REGEX MATCH "BOTAN_VERSION_MINOR ([0-9]*)" _ ${build})
    set(Botan_VERSION_MINOR ${CMAKE_MATCH_1})
    string(REGEX MATCH "BOTAN_VERSION_PATCH ([0-9]*)" _ ${build})
    set(Botan_VERSION_PATCH ${CMAKE_MATCH_1})
    set(Botan_VERSION "${Botan_VERSION_MAJOR}.${Botan_VERSION_MINOR}.${Botan_VERSION_PATCH}")
endif()

find_library(
    Botan_LIBRARY_RELEASE
    NAMES ${BOTAN_NAMES}
    HINTS ${PC_Botan_LIBRARY_DIRS}
    PATH_SUFFIXES release/lib lib
    DOC "The Botan (release) library")

if(WIN32 AND NOT MINGW)
    find_library(
        Botan_LIBRARY_DEBUG
        NAMES ${BOTAN_NAMES_DEBUG}
        HINTS ${PC_Botan_LIBRARY_DIRS}
        PATH_SUFFIXES debug/lib lib
        DOC "The Botan debug library")
endif()

include(SelectLibraryConfigurations)
select_library_configurations(Botan)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Botan
    REQUIRED_VARS Botan_LIBRARY Botan_INCLUDE_DIR
    VERSION_VAR Botan_VERSION)

if(Botan_FOUND)
    if(NOT TARGET Botan::botan)
        add_library(Botan::botan UNKNOWN IMPORTED)
    endif()
    if(Botan_LIBRARY_RELEASE)
        set_property(TARGET Botan::botan APPEND PROPERTY
                IMPORTED_CONFIGURATIONS RELEASE
        )
        set_target_properties(Botan::botan PROPERTIES
                IMPORTED_LOCATION_RELEASE ${Botan_LIBRARY_RELEASE})
    endif()
    if(Botan_LIBRARY_DEBUG)
        set_property(TARGET Botan::botan APPEND PROPERTY
                IMPORTED_CONFIGURATIONS DEBUG
        )
        set_target_properties(Botan::botan PROPERTIES
                IMPORTED_LOCATION_DEBUG ${Botan_LIBRARY_DEBUG})
    endif()
    set_target_properties(Botan::botan PROPERTIES
            INTERFACE_LINK_OPTIONS "${PC_Botan_LDFLAGS_OTHER}"
            INTERFACE_INCLUDE_DIRECTORIES "${Botan_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(Botan_INCLUDE_DIR Botan_LIBRARY_RELEASE Botan_LIBRARY_DEBUG)
