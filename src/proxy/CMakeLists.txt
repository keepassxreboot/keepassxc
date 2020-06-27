#  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

if(WITH_XC_BROWSER)
    set(proxy_SOURCES
        ../browser/BrowserShared.cpp
        keepassxc-proxy.cpp
        NativeMessagingProxy.cpp)

    # Alloc must be defined in a static library to prevent clashing with clang ASAN definitions
    add_library(proxy_alloc STATIC ../core/Alloc.cpp)
    target_link_libraries(proxy_alloc PRIVATE Qt5::Core ${sodium_LIBRARY_RELEASE})

    add_executable(keepassxc-proxy ${proxy_SOURCES})
    target_link_libraries(keepassxc-proxy proxy_alloc Qt5::Core Qt5::Network)
    install(TARGETS keepassxc-proxy
            BUNDLE DESTINATION . COMPONENT Runtime
            RUNTIME DESTINATION ${PROXY_INSTALL_DIR} COMPONENT Runtime)

    if(APPLE AND WITH_APP_BUNDLE)
        add_custom_command(TARGET keepassxc-proxy
                           POST_BUILD
                           COMMAND ${CMAKE_INSTALL_NAME_TOOL}
                                -change ${Qt5_PREFIX}/lib/QtCore.framework/Versions/5/QtCore
                                    "@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore"
                                -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore
                                    "@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore"
                                -change ${Qt5_PREFIX}/lib/QtNetwork.framework/Versions/5/QtNetwork
                                    "@executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork"
                                -change /usr/local/opt/qt/lib/QtNetwork.framework/Versions/5/QtNetwork
                                    "@executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork"
                                -change /usr/local/opt/libsodium/lib/libsodium.23.dylib
                                    "@executable_path/../Frameworks/libsodium.23.dylib"
                           keepassxc-proxy
                           COMMENT "Changing linking of keepassxc-proxy")

        # Copy app to staging directory for pre-install testing
        set(PROXY_APP_DIR "${CMAKE_BINARY_DIR}/src/${PROXY_INSTALL_DIR}")
        add_custom_command(TARGET keepassxc-proxy
                           POST_BUILD
                           COMMAND ${CMAKE_COMMAND} -E copy keepassxc-proxy ${PROXY_APP_DIR}/keepassxc-proxy
                           COMMENT "Copying keepassxc-proxy inside the application")
    endif()

    if(MINGW)
      target_link_libraries(keepassxc-proxy Wtsapi32.lib Ws2_32.lib)
    endif()
endif()
