#  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

if(KPXC_FEATURE_BROWSER)
    set(webproxy_SOURCES
            ../browser/BrowserShared.cpp
            keepassxc-webproxy.cpp
            WebProxy.cpp)

    # Alloc must be defined in a static library to prevent clashing with clang ASAN definitions
    add_library(webproxy_alloc STATIC ../core/Alloc.cpp)
    target_link_libraries(webproxy_alloc PRIVATE Qt6::Core ${BOTAN_LIBRARIES})

    add_executable(keepassxc-webproxy ${webproxy_SOURCES})
    target_link_libraries(keepassxc-webproxy webproxy_alloc Qt6::Core Qt6::Network Qt6::WebSockets)
    install(TARGETS keepassxc-webproxy
            BUNDLE DESTINATION . COMPONENT Runtime
            RUNTIME DESTINATION ${WEBPROXY_INSTALL_DIR} COMPONENT Runtime)

    if(APPLE AND WITH_APP_BUNDLE)
        set(WEBPROXY_APP_DIR "${CMAKE_BINARY_DIR}/src/${WEBPROXY_INSTALL_DIR}")
        add_custom_command(TARGET keepassxc-webproxy
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/keepassxc-webproxy ${WEBPROXY_APP_DIR}/keepassxc-webproxy
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src
                COMMENT "Copying keepassxc-webproxy into app bundle")

        set_property(GLOBAL APPEND PROPERTY
                _MACDEPLOYQT_EXTRA_BINARIES "${WEBPROXY_INSTALL_DIR}/keepassxc-webproxy")
    endif()

    if(WIN32)
        target_link_libraries(keepassxc-webproxy Wtsapi32.lib Ws2_32.lib)
    endif()
endif()
