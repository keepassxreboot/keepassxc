set(test_root "${CMAKE_CURRENT_BINARY_DIR}/test-find-qrencode")
file(REMOVE_RECURSE "${test_root}")
file(MAKE_DIRECTORY "${test_root}/include" "${test_root}/lib")
file(WRITE "${test_root}/include/qrencode.h" "")
file(WRITE "${test_root}/lib/qrencode.lib" "")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../cmake")
set(CMAKE_INCLUDE_PATH "${test_root}/include")
set(CMAKE_LIBRARY_PATH "${test_root}/lib")

find_package(QREncode REQUIRED)

if(NOT QRENCODE_LIBRARY STREQUAL "${test_root}/lib/qrencode.lib")
    message(FATAL_ERROR "Expected release-only QRencode library, found ${QRENCODE_LIBRARY}")
endif()

file(REMOVE_RECURSE "${test_root}")
