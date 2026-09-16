# vcpkg builds *deployqt (e.g. windeployqt.exe) for the host triplet only, which can
# differ from VCPKG_TARGET_TRIPLET when cross-compiling for Windows on Arm64 (e.g. a
# x64-windows host building an arm64-windows target). VCPKG_HOST_TRIPLET is set
# automatically by the vcpkg toolchain file and must be used as-is: it must not be
# re-derived from CMAKE_HOST_SYSTEM_PROCESSOR/NAME, which does not match vcpkg's
# triplet naming (e.g. "AMD64" vs. "x64").
function(kpxc_resolve_windows_deployqt_host_triplet out_var)
    if(NOT VCPKG_HOST_TRIPLET)
        message(FATAL_ERROR "VCPKG_HOST_TRIPLET is not set; cannot locate a Windows *deployqt tool.")
    endif()
    set(${out_var} "${VCPKG_HOST_TRIPLET}" PARENT_SCOPE)
endfunction()
