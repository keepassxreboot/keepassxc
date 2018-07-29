#  Copyright (C) 2017 KeePassXC Team
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 2 or (at your option)
#  version 3 of the License.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

set(cli_SOURCES
        Add.cpp
        Clip.cpp
        Create.cpp
        Command.cpp
        Diceware.cpp
        Edit.cpp
        Estimate.cpp
        Extract.cpp
        Generate.cpp
        List.cpp
        Locate.cpp
        Merge.cpp
        Remove.cpp
        Show.cpp)

add_library(cli STATIC ${cli_SOURCES})
target_link_libraries(cli Qt5::Core Qt5::Widgets)

add_executable(keepassxc-cli keepassxc-cli.cpp)
target_link_libraries(keepassxc-cli
        cli
        keepassx_core
        Qt5::Core
        ${GCRYPT_LIBRARIES}
        ${ARGON2_LIBRARIES}
        ${GPGERROR_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${ZXCVBN_LIBRARIES})

install(TARGETS keepassxc-cli
        BUNDLE DESTINATION . COMPONENT Runtime
        RUNTIME DESTINATION ${CLI_INSTALL_DIR} COMPONENT Runtime)

    if(APPLE AND WITH_APP_BUNDLE)
        set(CLI_BINARY_DIR "${CMAKE_BINARY_DIR}/src/cli/keepassxc-cli")
        set(CLI_APP_DIR "KeePassXC.app/Contents/MacOS/keepassxc-cli")
        add_custom_command(TARGET keepassxc-cli
                           POST_BUILD
                           COMMAND ${CMAKE_COMMAND} -E copy ${CLI_BINARY_DIR} ${CLI_APP_DIR}
                           WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src
                           COMMENT "Copying keepassxc-cli inside the application")

        add_custom_command(TARGET keepassxc-cli
                           POST_BUILD
                           COMMAND ${CMAKE_INSTALL_NAME_TOOL}
                                -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore
                                    "@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore"
                                -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui
                                    "@executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui"
                                -change /usr/local/opt/qt/lib/QtMacExtras.framework/Versions/5/QtMacExtras
                                    "@executable_path/../Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras"
                                -change /usr/local/opt/qt/lib/QtConcurrent.framework/Versions/5/QtConcurrent
                                    "@executable_path/../Frameworks/QtConcurrent.framework/Versions/5/QtConcurrent"
                                -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore
                                    "@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore"
                                -change /usr/local/opt/qt/lib/QtNetwork.framework/Versions/5/QtNetwork
                                    "@executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork"
                                -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets
                                    "@executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets"
                                -change /usr/local/opt/libgcrypt/lib/libgcrypt.20.dylib
                                    "@executable_path/../Frameworks/libgcrypt.20.dylib"
                                -change /usr/local/opt/argon2/lib/libargon2.1.dylib
                                    "@executable_path/../Frameworks/libargon2.1.dylib"
                                -change /usr/local/opt/libgpg-error/lib/libgpg-error.0.dylib
                                    "@executable_path/../Frameworks/libgpg-error.0.dylib"
                                -change /usr/local/opt/libsodium/lib/libsodium.23.dylib
                                    "@executable_path/../Frameworks/libsodium.23.dylib"
                           ${CLI_APP_DIR}
                           WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src
                           COMMENT "Changing linking of keepassxc-cli")
    endif()

if(APPLE OR UNIX)
    install(FILES keepassxc-cli.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1/)
    execute_process(COMMAND mandb -q)
endif()
