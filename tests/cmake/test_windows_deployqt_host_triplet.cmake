# Regression test for kpxc_resolve_windows_deployqt_host_triplet() (see
# cmake/KPXCWindowsDeployQt.cmake), which is required for locating windeployqt.exe
# when cross-compiling for the arm64-windows target on an x64-windows host.
include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/KPXCWindowsDeployQt.cmake")

if(TEST_CASE STREQUAL "host-triplet-set")
    # Native or cross builds alike: vcpkg's own VCPKG_HOST_TRIPLET must be used as-is.
    set(VCPKG_HOST_TRIPLET "x64-windows")
    kpxc_resolve_windows_deployqt_host_triplet(result)
    if(NOT result STREQUAL "x64-windows")
        message(FATAL_ERROR "Expected x64-windows, got: ${result}")
    endif()
elseif(TEST_CASE STREQUAL "host-triplet-arm64")
    set(VCPKG_HOST_TRIPLET "arm64-windows")
    kpxc_resolve_windows_deployqt_host_triplet(result)
    if(NOT result STREQUAL "arm64-windows")
        message(FATAL_ERROR "Expected arm64-windows, got: ${result}")
    endif()
elseif(TEST_CASE STREQUAL "host-triplet-unset")
    # Must fail loudly rather than silently falling back to a possibly wrong triplet.
    unset(VCPKG_HOST_TRIPLET)
    kpxc_resolve_windows_deployqt_host_triplet(result)
else()
    message(FATAL_ERROR "Unknown TEST_CASE: ${TEST_CASE}")
endif()
