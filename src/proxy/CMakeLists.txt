#  Copyright (C) 2017 Sami Vänttinen <sami.vanttinen@protonmail.com>
#  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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
    include_directories(${BROWSER_SOURCE_DIR})

    set(proxy_SOURCES
        keepassxc-proxy.cpp
        ${BROWSER_SOURCE_DIR}/NativeMessagingBase.cpp
        NativeMessagingHost.cpp)

    add_library(proxy STATIC ${proxy_SOURCES})
    target_link_libraries(proxy Qt5::Core Qt5::Network)
    add_executable(keepassxc-proxy keepassxc-proxy.cpp)
    target_link_libraries(keepassxc-proxy proxy)

    install(TARGETS keepassxc-proxy
            BUNDLE DESTINATION . COMPONENT Runtime
            RUNTIME DESTINATION ${PROXY_INSTALL_DIR} COMPONENT Runtime)

    if(APPLE AND WITH_APP_BUNDLE)
        set(PROXY_BINARY_DIR "${CMAKE_BINARY_DIR}/src/proxy/keepassxc-proxy")
        set(PROXY_APP_DIR "KeePassXC.app/Contents/MacOS/keepassxc-proxy")
        add_custom_command(TARGET keepassxc-proxy
                           POST_BUILD
                           COMMAND ${CMAKE_COMMAND} -E copy ${PROXY_BINARY_DIR} ${PROXY_APP_DIR}
                           WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src
                           COMMENT "Copying keepassxc-proxy inside the application")

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
                           ${PROXY_APP_DIR}
                           WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src
                           COMMENT "Changing linking of keepassxc-proxy")
    endif()
    if(MINGW)
      target_link_libraries(keepassxc-proxy Wtsapi32.lib Ws2_32.lib)
    endif()
endif()
