# QUAZIP_FOUND                   - QuaZip library was found
# QUAZIP_INCLUDE_DIR             - Path to QuaZip include dir
# QUAZIP_INCLUDE_DIRS            - Path to QuaZip and zlib include dir (combined from QUAZIP_INCLUDE_DIR + ZLIB_INCLUDE_DIR)
# QUAZIP_LIBRARIES               - List of QuaZip libraries
# QUAZIP_ZLIB_INCLUDE_DIR        - The include dir of zlib headers

if(MINGW)
    find_library(QUAZIP_LIBRARIES libquazip5)
    find_path(QUAZIP_INCLUDE_DIR quazip.h PATH_SUFFIXES quazip5)
    find_path(QUAZIP_ZLIB_INCLUDE_DIR zlib.h)
else()
    find_library(QUAZIP_LIBRARIES
        NAMES quazip5 quazip
	PATHS /usr/lib /usr/lib64 /usr/local/lib
    )
    find_path(QUAZIP_INCLUDE_DIR quazip.h
	PATHS /usr/include /usr/local/include
        PATH_SUFFIXES quazip5 quazip
    )
    find_path(QUAZIP_ZLIB_INCLUDE_DIR zlib.h PATHS /usr/include /usr/local/include)
endif()
include(FindPackageHandleStandardArgs)
set(QUAZIP_INCLUDE_DIRS ${QUAZIP_INCLUDE_DIR} ${QUAZIP_ZLIB_INCLUDE_DIR})
find_package_handle_standard_args(QUAZIP DEFAULT_MSG QUAZIP_LIBRARIES QUAZIP_INCLUDE_DIR QUAZIP_ZLIB_INCLUDE_DIR QUAZIP_INCLUDE_DIRS)
