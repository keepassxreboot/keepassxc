#  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )

configure_file( config-keepassx.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-keepassx.h )

set(keepassx_SOURCES
    core/Database.cpp
    core/Entry.cpp
    core/Group.cpp
    core/Metadata.cpp
    core/TimeInfo.cpp
    core/Uuid.cpp
    crypto/Crypto.cpp
    crypto/CryptoHash.cpp
    crypto/Random.cpp
    crypto/SymmetricCipher.cpp
    format/KeePass2XmlReader.cpp
    format/KeePass2XmlWriter.cpp
    gui/DatabaseWidget.cpp
    gui/EntryModel.cpp
    gui/EntryView.cpp
    gui/GroupModel.cpp
    gui/GroupView.cpp
)

automoc4_add_library( keepassx_core STATIC ${keepassx_SOURCES} )

automoc4_add_executable( ${PROGNAME} WIN32 MACOSX_BUNDLE main.cpp )
target_link_libraries( ${PROGNAME} keepassx_core ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${LIBGCRYPT_LIBS} )
