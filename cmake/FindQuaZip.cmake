# QUAZIP_FOUND                   - QuaZip library was found
# QUAZIP_INCLUDE_DIRS            - Path to QuaZip include dir
# QUAZIP_LIBRARIES               - List of QuaZip libraries

if(WIN32)
    find_library(QUAZIP_LIBRARIES libquazip5)
    find_path(QUAZIP_INCLUDE_DIRS quazip.h PATH_SUFFIXES quazip5)
elseif(APPLE)
    find_library(QUAZIP_LIBRARIES quazip1-qt5)
    find_path(QUAZIP_INCLUDE_DIRS quazip.h PATH_SUFFIXES quazip)
else()
    # Try pkgconfig first
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(QUAZIP QUIET quazip1-qt5)
    endif()
    if(NOT QUAZIP_FOUND)
        # Try to find QuaZip version 0.x
        find_library(QUAZIP_LIBRARIES
            NAMES quazip5 quazip
            PATHS /usr/lib /usr/lib64 /usr/local/lib
        )
        find_path(QUAZIP_INCLUDE_DIRS quazip.h
            PATHS /usr/include /usr/local/include
            PATH_SUFFIXES quazip5 quazip
        )
    endif()
endif()

mark_as_advanced(QUAZIP_LIBRARIES QUAZIP_INCLUDE_DIRS)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QuaZip DEFAULT_MSG QUAZIP_LIBRARIES QUAZIP_INCLUDE_DIRS)
