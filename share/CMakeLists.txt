#  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

file(GLOB DATABASE_ICONS icons/database/*.png)

install(FILES ${DATABASE_ICONS} DESTINATION ${DATA_INSTALL_DIR}/icons/database)

if(UNIX AND NOT APPLE)
    install(DIRECTORY icons/application/ DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor
            FILES_MATCHING PATTERN "keepassx*.png" PATTERN "keepassx*.svgz")
    install(DIRECTORY icons/application/ DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor
            FILES_MATCHING PATTERN "application-x-keepassxc.png" PATTERN "application-x-keepassxc.svgz")
    install(FILES linux/keepassxc.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES linux/keepassxc.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)
endif(UNIX AND NOT APPLE)

if(APPLE)
  install(FILES macosx/keepassxc.icns DESTINATION ${DATA_INSTALL_DIR})
endif()

install(DIRECTORY icons/application/ DESTINATION ${DATA_INSTALL_DIR}/icons/application
        FILES_MATCHING PATTERN "*.png" PATTERN "*.svgz")

add_custom_target(icons
                  COMMAND png2icns macosx/keepassxc.icns
                    icons/application/16x16/apps/keepassxc.png
                    icons/application/32x32/apps/keepassxc.png
                    icons/application/48x48/apps/keepassxc.png
                    icons/application/128x128/apps/keepassxc.png
                    icons/application/256x256/apps/keepassxc.png
                  COMMAND icotool -c -o windows/keepassxc.ico
                    icons/application/16x16/apps/keepassxc.png
                    icons/application/24x24/apps/keepassxc.png
                    icons/application/32x32/apps/keepassxc.png
                    icons/application/48x48/apps/keepassxc.png
                    icons/application/64x64/apps/keepassxc.png
                    icons/application/128x128/apps/keepassxc.png
                    icons/application/256x256/apps/keepassxc.png
                  VERBATIM
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
