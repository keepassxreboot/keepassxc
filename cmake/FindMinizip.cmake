# MINIZIP_FOUND                   - Minizip library was found
# MINIZIP_INCLUDE_DIR             - Path to Minizip include dir
# MINIZIP_LIBRARIES               - List of Minizip libraries

find_library(MINIZIP_LIBRARIES NAMES minizip libminizip)
find_path(MINIZIP_INCLUDE_DIR zip.h PATH_SUFFIXES minizip)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Minizip DEFAULT_MSG MINIZIP_LIBRARIES MINIZIP_INCLUDE_DIR)
