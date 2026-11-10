# Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 or (at your option)
# version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Generate only after deployment. CPack invokes this again after its signing hooks.
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
find_package(Syft REQUIRED)
execute_process(COMMAND "${Python3_EXECUTABLE}" -c "import spdx_tools.spdx.parser.parse_anything"
    RESULT_VARIABLE _sbom_python_result ERROR_QUIET)
if(NOT _sbom_python_result EQUAL 0)
    message(FATAL_ERROR "WITH_SBOM requires spdx-tools in ${Python3_EXECUTABLE}: python -m pip install spdx-tools")
endif()

# Walk dependencies of all installed targets.
get_property(pending GLOBAL PROPERTY KEEPASSXC_INSTALLED_TARGETS)
set(visited "")
set(link_inputs "")
while(pending)
    list(POP_FRONT pending target)
    if(NOT TARGET "${target}" OR target IN_LIST visited)
        continue()
    endif()
    list(APPEND visited "${target}")
    get_target_property(type "${target}" TYPE)
    get_target_property(imported "${target}" IMPORTED)
    if(imported AND type MATCHES "^(STATIC|SHARED|UNKNOWN)_LIBRARY$")
        string(APPEND link_inputs "LIBRARY|$<TARGET_FILE:${target}>\n")
    endif()
    get_target_property(source "${target}" SBOM_SOURCE_PATH)
    if(source)
        get_target_property(version "${target}" VERSION)
        if(NOT version)
            set(version "")
        endif()
        string(APPEND link_inputs "VENDORED|${target}|${source}|${version}\n")
    endif()
    foreach(property LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
        get_target_property(links "${target}" "${property}")
        foreach(link IN LISTS links)
            # Static target interfaces wrap private dependencies in LINK_ONLY.
            string(REGEX REPLACE "^\\$<(LINK_ONLY|BUILD_INTERFACE):([^<>]*)>$" "\\2" link "${link}")
            if(TARGET "${link}")
                list(APPEND pending "${link}")
            elseif(IS_ABSOLUTE "${link}")
                string(APPEND link_inputs "LIBRARY|${link}\n")
            endif()
        endforeach()
    endforeach()
endwhile()
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/sbom-link-inputs-$<CONFIG>.txt" CONTENT "${link_inputs}")
configure_file("${CMAKE_SOURCE_DIR}/cmake/GenerateSbom.cmake.in"
    "${CMAKE_BINARY_DIR}/GenerateSbom.cmake" @ONLY)
set(SBOM_DEPLOY_DIR "" CACHE PATH "Deployed folder to scan with the sbom target (e.g. an AppDir)")
add_custom_target(sbom
    COMMAND "${CMAKE_COMMAND}" "-DSBOM_DEPLOY_DIR=${SBOM_DEPLOY_DIR}"
        "-DSBOM_CONFIG=$<CONFIG>"
        -P "${CMAKE_BINARY_DIR}/GenerateSbom.cmake"
    COMMENT "Generating deployed application SBOM" VERBATIM)
if(WITH_TESTS)
    add_test(NAME test_deployment_sbom
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/sbom/test_generate_sbom.py")
endif()

# AppImage deployment happens outside cmake --install; release-tool runs the
# sbom target after linuxdeploy. Other installations are scanned at the very end.
if(NOT KEEPASSXC_DIST_TYPE STREQUAL "AppImage")
    # CPack installs components separately. Runtime precedes Unspecified, which
    # runs macdeployqt, so only the CPack pre-build hook can scan the full bundle.
    install(CODE "
        if(NOT CMAKE_INSTALL_COMPONENT)
            include(\"${CMAKE_BINARY_DIR}/GenerateSbom.cmake\")
        endif()
    " COMPONENT Runtime)
endif()
