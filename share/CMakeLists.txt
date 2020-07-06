#  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
#  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

add_subdirectory(translations)

file(GLOB wordlists_files "wordlists/*.wordlist")
install(FILES ${wordlists_files} DESTINATION ${DATA_INSTALL_DIR}/wordlists)

# Copy wordlists to build dir for use in tests
file(COPY "wordlists" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

if(UNIX AND NOT APPLE AND NOT HAIKU)
    install(DIRECTORY icons/application/ DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor
            FILES_MATCHING PATTERN "keepassx*.png" PATTERN "keepassx*.svg"
            PATTERN "status" EXCLUDE PATTERN "actions" EXCLUDE PATTERN "categories" EXCLUDE)
    install(DIRECTORY icons/application/ DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor
            FILES_MATCHING PATTERN "application-x-keepassxc.png" PATTERN "application-x-keepassxc.svg"
            PATTERN "status" EXCLUDE PATTERN "actions" EXCLUDE PATTERN "categories" EXCLUDE)
    install(FILES linux/org.keepassxc.KeePassXC.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES linux/org.keepassxc.KeePassXC.appdata.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
    install(FILES linux/keepassxc.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)
endif(UNIX AND NOT APPLE AND NOT HAIKU)

if(APPLE)
  install(FILES macosx/keepassxc.icns DESTINATION ${DATA_INSTALL_DIR})
endif()

install(FILES icons/application/256x256/apps/keepassxc.png DESTINATION ${DATA_INSTALL_DIR}/icons/application/256x256/apps)

add_custom_target(icons)
add_custom_command(TARGET icons
        COMMAND bash ./icons/minify.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
if(APPLE)
    add_custom_command(TARGET icons
            COMMAND png2icns macosx/keepassxc.icns icons/application/256x256/apps/keepassxc.png
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# ICO for Windows
add_custom_command(TARGET icons
    COMMAND bash ./windows/create-ico.sh icons/application/scalable/apps/keepassxc.svg windows/keepassxc.ico
    COMMAND bash ./windows/create-ico.sh icons/application/scalable/mimetypes/application-x-keepassxc.svg windows/keepassxc-kdbx.ico
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
