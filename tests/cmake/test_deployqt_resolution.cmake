include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/KPXCDeployQt.cmake")

if(TEST_CASE STREQUAL "windows-native-x64")
    set(test_root "${CMAKE_CURRENT_BINARY_DIR}/test-deployqt-native")
    set(host_tools_dir "${test_root}/vcpkg_installed/x64-windows/tools/Qt6/bin")
    file(REMOVE_RECURSE "${test_root}" "${CMAKE_BINARY_DIR}/kpxc-deployqt")
    file(MAKE_DIRECTORY
            "${host_tools_dir}"
            "${test_root}/vcpkg_installed/x64-windows-release")
    file(WRITE "${host_tools_dir}/windeployqt.exe" "")
    file(WRITE "${host_tools_dir}/qtpaths.exe" "")

    kpxc_resolve_vcpkg_deployqt(
            REQUIRE_EXISTS
            HOST_SYSTEM_NAME Windows
            HOST_SYSTEM_PROCESSOR AMD64
            VCPKG_INSTALLED_DIR "${test_root}/vcpkg_installed"
            VCPKG_TARGET_TRIPLET x64-windows-release
            TARGET_QT_PREFIX "${test_root}/vcpkg_installed/x64-windows-release"
            DEPLOYQT_EXE_NAME windeployqt.exe
            OUT_EXECUTABLE executable
            OUT_ARGUMENTS arguments
            OUT_HOST_TRIPLET host_triplet)
    if(NOT host_triplet STREQUAL "x64-windows")
        message(FATAL_ERROR "Expected x64-windows host triplet, found ${host_triplet}")
    endif()
    if(NOT executable STREQUAL
            "${test_root}/vcpkg_installed/x64-windows/tools/Qt6/bin/windeployqt.exe")
        message(FATAL_ERROR "Unexpected host deployment tool: ${executable}")
    endif()
    list(GET arguments 0 argument_name)
    list(GET arguments 1 argument_value)
    if(NOT argument_name STREQUAL "--qtpaths")
        message(FATAL_ERROR "Expected --qtpaths argument, found ${argument_name}")
    endif()
    if(NOT argument_value STREQUAL "${CMAKE_BINARY_DIR}/kpxc-deployqt/qtpaths.bat")
        message(FATAL_ERROR "Unexpected qtpaths wrapper: ${argument_value}")
    endif()
    file(READ "${CMAKE_BINARY_DIR}/kpxc-deployqt/target_qt.conf" qt_conf)
    if(NOT qt_conf MATCHES "Prefix=${test_root}/vcpkg_installed/x64-windows-release")
        message(FATAL_ERROR "Target Qt prefix missing from qt.conf: ${qt_conf}")
    endif()
    file(REMOVE_RECURSE "${test_root}" "${CMAKE_BINARY_DIR}/kpxc-deployqt")
elseif(TEST_CASE STREQUAL "windows-x64-to-arm64")
    kpxc_resolve_vcpkg_deployqt(
            HOST_SYSTEM_NAME Windows
            HOST_SYSTEM_PROCESSOR AMD64
            VCPKG_INSTALLED_DIR C:/vcpkg_installed
            VCPKG_TARGET_TRIPLET arm64-windows-release
            TARGET_QT_PREFIX C:/vcpkg_installed/arm64-windows-release
            DEPLOYQT_EXE_NAME windeployqt.exe
            OUT_EXECUTABLE executable
            OUT_ARGUMENTS arguments
            OUT_HOST_TRIPLET host_triplet)
    if(NOT host_triplet STREQUAL "x64-windows")
        message(FATAL_ERROR "Expected x64-windows host triplet, found ${host_triplet}")
    endif()
    if(NOT executable STREQUAL
            "C:/vcpkg_installed/x64-windows/tools/Qt6/bin/windeployqt.exe")
        message(FATAL_ERROR "Unexpected host deployment tool: ${executable}")
    endif()
    if(NOT arguments STREQUAL
            "--qtpaths;C:/vcpkg_installed/arm64-windows-release/tools/Qt6/bin/qtpaths.bat")
        message(FATAL_ERROR "Unexpected target deployment arguments: ${arguments}")
    endif()
elseif(TEST_CASE STREQUAL "macos-x64-to-arm64")
    kpxc_resolve_vcpkg_deployqt(
            HOST_SYSTEM_NAME Darwin
            HOST_SYSTEM_PROCESSOR x86_64
            VCPKG_INSTALLED_DIR /vcpkg_installed
            VCPKG_TARGET_TRIPLET arm64-osx
            TARGET_QT_PREFIX /vcpkg_installed/arm64-osx
            DEPLOYQT_EXE_NAME macdeployqt
            OUT_EXECUTABLE executable
            OUT_ARGUMENTS arguments
            OUT_HOST_TRIPLET host_triplet)
    if(NOT host_triplet STREQUAL "x64-osx")
        message(FATAL_ERROR "Expected x64-osx host triplet, found ${host_triplet}")
    endif()
    if(NOT executable STREQUAL
            "/vcpkg_installed/x64-osx/tools/Qt6/bin/macdeployqt")
        message(FATAL_ERROR "Unexpected host deployment tool: ${executable}")
    endif()
    if(NOT arguments STREQUAL "-libpath=/vcpkg_installed/arm64-osx/lib")
        message(FATAL_ERROR "Unexpected target deployment arguments: ${arguments}")
    endif()
else()
    message(FATAL_ERROR "Unknown TEST_CASE: ${TEST_CASE}")
endif()
