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

file(GLOB DATABASE_ICONS icons/database/*.png)

install(FILES ${DATABASE_ICONS} DESTINATION ${DATA_INSTALL_DIR}/icons/database)

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

install(DIRECTORY docs/ DESTINATION ${DATA_INSTALL_DIR}/docs FILES_MATCHING PATTERN "*.pdf")

install(DIRECTORY wizard/ DESTINATION ${DATA_INSTALL_DIR}/wizard FILES_MATCHING PATTERN "*.png")

install(DIRECTORY icons/application/ DESTINATION ${DATA_INSTALL_DIR}/icons/application
        FILES_MATCHING PATTERN "*.png" PATTERN "*.svg")

add_custom_target(icons
    # SVG to PNGs for KeePassXC
    COMMAND inkscape -z -w 16 -h 16
        icons/application/scalable/apps/keepassxc.svg -e icons/application/16x16/apps/keepassxc.png
    COMMAND inkscape -z -w  24 -h 24
        icons/application/scalable/apps/keepassxc.svg -e icons/application/24x24/apps/keepassxc.png
    COMMAND inkscape -z -w  32 -h 32
        icons/application/scalable/apps/keepassxc.svg -e icons/application/32x32/apps/keepassxc.png
    COMMAND inkscape -z -w 48 -h 48
        icons/application/scalable/apps/keepassxc.svg -e icons/application/48x48/apps/keepassxc.png
    COMMAND inkscape -z -w 64 -h 64
        icons/application/scalable/apps/keepassxc.svg -e icons/application/64x64/apps/keepassxc.png
    COMMAND inkscape -z -w 128 -h 128
        icons/application/scalable/apps/keepassxc.svg -e icons/application/128x128/apps/keepassxc.png
    COMMAND inkscape -z -w 256 -h 256
        icons/application/scalable/apps/keepassxc.svg -e icons/application/256x256/apps/keepassxc.png

    # SVG to PNGs for KeePassXC
    COMMAND inkscape -z -w 16 -h 16
        icons/application/scalable/apps/keepassxc-dark.svg -e icons/application/16x16/apps/keepassxc-dark.png
    COMMAND inkscape -z -w  24 -h 24
        icons/application/scalable/apps/keepassxc-dark.svg -e icons/application/24x24/apps/keepassxc-dark.png
    COMMAND inkscape -z -w  32 -h 32
        icons/application/scalable/apps/keepassxc-dark.svg -e icons/application/32x32/apps/keepassxc-dark.png
    COMMAND inkscape -z -w 48 -h 48
        icons/application/scalable/apps/keepassxc-dark.svg -e icons/application/48x48/apps/keepassxc-dark.png
    COMMAND inkscape -z -w 64 -h 64
        icons/application/scalable/apps/keepassxc-dark.svg -e icons/application/64x64/apps/keepassxc-dark.png
    COMMAND inkscape -z -w 128 -h 128
        icons/application/scalable/apps/keepassxc-dark.svg -e icons/application/128x128/apps/keepassxc-dark.png
    COMMAND inkscape -z -w 256 -h 256
        icons/application/scalable/apps/keepassxc-dark.svg -e icons/application/256x256/apps/keepassxc-dark.png

    # SVG to PNGs for KeePassXC
    COMMAND inkscape -z -w 16 -h 16
        icons/application/scalable/apps/keepassxc-locked.svg -e icons/application/16x16/apps/keepassxc-locked.png
    COMMAND inkscape -z -w  24 -h 24
        icons/application/scalable/apps/keepassxc-locked.svg -e icons/application/24x24/apps/keepassxc-locked.png
    COMMAND inkscape -z -w  32 -h 32
        icons/application/scalable/apps/keepassxc-locked.svg -e icons/application/32x32/apps/keepassxc-locked.png
    COMMAND inkscape -z -w 48 -h 48
        icons/application/scalable/apps/keepassxc-locked.svg -e icons/application/48x48/apps/keepassxc-locked.png
    COMMAND inkscape -z -w 64 -h 64
        icons/application/scalable/apps/keepassxc-locked.svg -e icons/application/64x64/apps/keepassxc-locked.png
    COMMAND inkscape -z -w 128 -h 128
        icons/application/scalable/apps/keepassxc-locked.svg -e icons/application/128x128/apps/keepassxc-locked.png
    COMMAND inkscape -z -w 256 -h 256
        icons/application/scalable/apps/keepassxc-locked.svg -e icons/application/256x256/apps/keepassxc-locked.png

    # SVG to PNGs for KeePassXC
    COMMAND inkscape -z -w 16 -h 16
        icons/application/scalable/apps/keepassxc-unlocked.svg -e icons/application/16x16/apps/keepassxc-unlocked.png
    COMMAND inkscape -z -w  24 -h 24
        icons/application/scalable/apps/keepassxc-unlocked.svg -e icons/application/24x24/apps/keepassxc-unlocked.png
    COMMAND inkscape -z -w  32 -h 32
        icons/application/scalable/apps/keepassxc-unlocked.svg -e icons/application/32x32/apps/keepassxc-unlocked.png
    COMMAND inkscape -z -w 48 -h 48
        icons/application/scalable/apps/keepassxc-unlocked.svg -e icons/application/48x48/apps/keepassxc-unlocked.png
    COMMAND inkscape -z -w 64 -h 64
        icons/application/scalable/apps/keepassxc-unlocked.svg -e icons/application/64x64/apps/keepassxc-unlocked.png
    COMMAND inkscape -z -w 128 -h 128
        icons/application/scalable/apps/keepassxc-unlocked.svg -e icons/application/128x128/apps/keepassxc-unlocked.png
    COMMAND inkscape -z -w 256 -h 256
        icons/application/scalable/apps/keepassxc-unlocked.svg -e icons/application/256x256/apps/keepassxc-unlocked.png

    # SVG to PNGs for KeePassXC MIME-Type
    COMMAND inkscape -z -w 16 -h 16
        icons/application/scalable/mimetypes/application-x-keepassxc.svg -e icons/application/16x16/mimetypes/application-x-keepassxc.png
    COMMAND inkscape -z -w 22 -h 22
        icons/application/scalable/mimetypes/application-x-keepassxc.svg -e icons/application/22x22/mimetypes/application-x-keepassxc.png
    COMMAND inkscape -z -w 32 -h 32
        icons/application/scalable/mimetypes/application-x-keepassxc.svg -e icons/application/32x32/mimetypes/application-x-keepassxc.png
    COMMAND inkscape -z -w 64 -h 64
        icons/application/scalable/mimetypes/application-x-keepassxc.svg -e icons/application/64x64/mimetypes/application-x-keepassxc.png
    COMMAND inkscape -z -w 128 -h 128
        icons/application/scalable/mimetypes/application-x-keepassxc.svg -e icons/application/128x128/mimetypes/application-x-keepassxc.png

    # Shrink PNGs using pngcrush
    COMMAND bash ./crushpng.sh icons

    # ICNS for MacOS
    COMMAND png2icns macosx/keepassxc.icns
        icons/application/16x16/apps/keepassxc.png
        icons/application/32x32/apps/keepassxc.png
        icons/application/48x48/apps/keepassxc.png
        icons/application/128x128/apps/keepassxc.png
        icons/application/256x256/apps/keepassxc.png

    # ICO for Windows
    COMMAND bash ./windows/create-ico.sh icons/application/scalable/apps/keepassxc.svg windows/keepassxc.ico
    COMMAND bash ./windows/create-ico.sh icons/application/scalable/mimetypes/application-x-keepassxc.svg windows/keepassxc-kdbx.ico

    VERBATIM
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
