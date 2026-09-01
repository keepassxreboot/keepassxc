include_guard(GLOBAL)

function(kpxc_resolve_vcpkg_deployqt)
    set(options REQUIRE_EXISTS)
    set(one_value_args
            HOST_SYSTEM_NAME
            HOST_SYSTEM_PROCESSOR
            VCPKG_INSTALLED_DIR
            VCPKG_HOST_TRIPLET
            VCPKG_TARGET_TRIPLET
            TARGET_QT_PREFIX
            DEPLOYQT_EXE_NAME
            OUT_EXECUTABLE
            OUT_ARGUMENTS
            OUT_HOST_TRIPLET)
    cmake_parse_arguments(ARG "${options}" "${one_value_args}" "" ${ARGN})

    foreach(required
            HOST_SYSTEM_NAME
            HOST_SYSTEM_PROCESSOR
            VCPKG_INSTALLED_DIR
            VCPKG_TARGET_TRIPLET
            TARGET_QT_PREFIX
            DEPLOYQT_EXE_NAME
            OUT_EXECUTABLE
            OUT_ARGUMENTS
            OUT_HOST_TRIPLET)
        if(NOT ARG_${required})
            message(FATAL_ERROR "kpxc_resolve_vcpkg_deployqt requires ${required}")
        endif()
    endforeach()

    set(host_triplet "${ARG_VCPKG_HOST_TRIPLET}")
    if(NOT host_triplet)
        if(ARG_HOST_SYSTEM_NAME STREQUAL "Windows")
            if(ARG_HOST_SYSTEM_PROCESSOR MATCHES "^(AMD64|amd64|x64|x86_64)$")
                set(host_triplet x64-windows)
            elseif(ARG_HOST_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64)$")
                set(host_triplet arm64-windows)
            endif()
        elseif(ARG_HOST_SYSTEM_NAME STREQUAL "Darwin")
            if(ARG_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
                set(host_triplet x64-osx)
            elseif(ARG_HOST_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64)$")
                set(host_triplet arm64-osx)
            endif()
        endif()
        if(NOT host_triplet)
            message(FATAL_ERROR
                    "Unsupported deployqt host: OS=${ARG_HOST_SYSTEM_NAME}, "
                    "processor=${ARG_HOST_SYSTEM_PROCESSOR}")
        endif()
    endif()

    set(host_tools_dir
            "${ARG_VCPKG_INSTALLED_DIR}/${host_triplet}/tools/Qt6/bin")
    set(deployqt_executable "${host_tools_dir}/${ARG_DEPLOYQT_EXE_NAME}")
    if(ARG_REQUIRE_EXISTS AND NOT EXISTS "${deployqt_executable}")
        message(FATAL_ERROR
                "The Qt deployment tool was not installed for host triplet "
                "${host_triplet}: ${deployqt_executable}")
    endif()

    if(ARG_HOST_SYSTEM_NAME STREQUAL "Windows")
        string(REGEX REPLACE "-release$" "" normalized_target_triplet
                "${ARG_VCPKG_TARGET_TRIPLET}")
        if(host_triplet STREQUAL normalized_target_triplet)
            set(host_qtpaths_candidates
                    "${host_tools_dir}/qtpaths.exe"
                    "${host_tools_dir}/qtpaths6.exe")
            list(GET host_qtpaths_candidates 0 host_qtpaths)
            if(ARG_REQUIRE_EXISTS)
                unset(host_qtpaths)
                foreach(candidate IN LISTS host_qtpaths_candidates)
                    if(EXISTS "${candidate}")
                        set(host_qtpaths "${candidate}")
                        break()
                    endif()
                endforeach()
                if(NOT host_qtpaths)
                    message(FATAL_ERROR
                            "No host qtpaths executable was found for triplet "
                            "${host_triplet} under ${host_tools_dir}")
                endif()
            endif()

            set(wrapper_dir "${CMAKE_BINARY_DIR}/kpxc-deployqt")
            set(target_qt_conf "${wrapper_dir}/target_qt.conf")
            set(target_qtpaths "${wrapper_dir}/qtpaths.bat")
            file(MAKE_DIRECTORY "${wrapper_dir}")
            file(WRITE "${target_qt_conf}"
                    "[Paths]\nPrefix=${ARG_TARGET_QT_PREFIX}\n")
            file(TO_NATIVE_PATH "${host_qtpaths}" host_qtpaths_native)
            file(TO_NATIVE_PATH "${target_qt_conf}" target_qt_conf_native)
            file(WRITE "${target_qtpaths}"
                    "@echo off\r\n"
                    "\"${host_qtpaths_native}\" -qtconf \"${target_qt_conf_native}\" %*\r\n")
        else()
            set(target_tools_dir
                    "${ARG_VCPKG_INSTALLED_DIR}/${ARG_VCPKG_TARGET_TRIPLET}/tools/Qt6/bin")
            set(qtpaths_candidates
                    "${target_tools_dir}/qtpaths.bat"
                    "${target_tools_dir}/qtpaths6.bat")
            list(GET qtpaths_candidates 0 target_qtpaths)
            if(ARG_REQUIRE_EXISTS)
                unset(target_qtpaths)
                foreach(candidate IN LISTS qtpaths_candidates)
                    if(EXISTS "${candidate}")
                        set(target_qtpaths "${candidate}")
                        break()
                    endif()
                endforeach()
                if(NOT target_qtpaths)
                    message(FATAL_ERROR
                            "No target qtpaths wrapper was found for triplet "
                            "${ARG_VCPKG_TARGET_TRIPLET} under ${target_tools_dir}")
                endif()
            endif()
        endif()
        set(deployqt_arguments --qtpaths "${target_qtpaths}")
    elseif(ARG_HOST_SYSTEM_NAME STREQUAL "Darwin")
        set(deployqt_arguments "-libpath=${ARG_TARGET_QT_PREFIX}/lib")
    else()
        message(FATAL_ERROR
                "Unsupported deployqt host: OS=${ARG_HOST_SYSTEM_NAME}, "
                "processor=${ARG_HOST_SYSTEM_PROCESSOR}")
    endif()

    set(${ARG_OUT_EXECUTABLE} "${deployqt_executable}" PARENT_SCOPE)
    set(${ARG_OUT_ARGUMENTS} "${deployqt_arguments}" PARENT_SCOPE)
    set(${ARG_OUT_HOST_TRIPLET} "${host_triplet}" PARENT_SCOPE)
endfunction()
