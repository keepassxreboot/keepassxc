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
    core/Config.cpp
    core/Database.cpp
    core/DatabaseIcons.cpp
    core/DataPath.cpp
    core/Endian.cpp
    core/Entry.cpp
    core/EntryAttachments.cpp
    core/EntryAttributes.cpp
    core/Group.cpp
    core/Metadata.cpp
    core/TimeInfo.cpp
    core/Tools.cpp
    core/Uuid.cpp
    crypto/Crypto.cpp
    crypto/CryptoHash.cpp
    crypto/Random.cpp
    crypto/salsa20/ecrypt-config.h
    crypto/salsa20/ecrypt-machine.h
    crypto/salsa20/ecrypt-portable.h
    crypto/salsa20/ecrypt-sync.h
    crypto/salsa20/salsa20.c
    crypto/SymmetricCipher.cpp
    crypto/SymmetricCipherBackend.h
    crypto/SymmetricCipherGcrypt.cpp
    crypto/SymmetricCipherSalsa20.cpp
    format/KeePass1.h
    format/KeePass1Reader.cpp
    format/KeePass2.h
    format/KeePass2RandomStream.cpp
    format/KeePass2Reader.cpp
    format/KeePass2Writer.cpp
    format/KeePass2XmlReader.cpp
    format/KeePass2XmlWriter.cpp
    gui/AboutDialog.cpp
    gui/ChangeMasterKeyWidget.cpp
    gui/DatabaseOpenDialog.cpp
    gui/DatabaseSettingsWidget.cpp
    gui/DatabaseTabWidget.cpp
    gui/DatabaseWidget.cpp
    gui/DialogyWidget.cpp
    gui/DragTabBar.cpp
    gui/EditWidget.cpp
    gui/EditWidgetIcons.cpp
    gui/FileDialog.cpp
    gui/IconModels.cpp
    gui/KeePass1OpenDialog.cpp
    gui/MainWindow.cpp
    gui/entry/EditEntryWidget.cpp
    gui/entry/EntryAttachmentsModel.cpp
    gui/entry/EntryAttributesModel.cpp
    gui/entry/EntryHistoryModel.cpp
    gui/entry/EntryModel.cpp
    gui/entry/EntryView.cpp
    gui/group/EditGroupWidget.cpp
    gui/group/GroupModel.cpp
    gui/group/GroupView.cpp
    keys/CompositeKey.cpp
    keys/FileKey.cpp
    keys/Key.h
    keys/PasswordKey.cpp
    streams/HashedBlockStream.cpp
    streams/LayeredStream.cpp
    streams/qtiocompressor.cpp
    streams/SymmetricCipherStream.cpp
)

set(keepassx_MOC
    core/Database.h
    core/Entry.h
    core/EntryAttachments.h
    core/EntryAttributes.h
    core/Group.h
    core/Metadata.h
    gui/AboutDialog.h
    gui/ChangeMasterKeyWidget.h
    gui/DatabaseOpenDialog.h
    gui/DatabaseSettingsWidget.h
    gui/DatabaseTabWidget.h
    gui/DatabaseWidget.h
    gui/DialogyWidget.h
    gui/DragTabBar.h
    gui/EditWidget.h
    gui/EditWidgetIcons.h
    gui/IconModels.h
    gui/MainWindow.h
    gui/entry/EditEntryWidget.h
    gui/entry/EntryAttachmentsModel.h
    gui/entry/EntryAttributesModel.h
    gui/entry/EntryHistoryModel.h
    gui/entry/EntryModel.h
    gui/entry/EntryView.h
    gui/group/EditGroupWidget.h
    gui/group/GroupModel.h
    gui/group/GroupView.h
    keys/CompositeKey_p.h
    streams/HashedBlockStream.h
    streams/LayeredStream.h
    streams/qtiocompressor.h
    streams/SymmetricCipherStream.h
)

set(keepassx_FORMS
    gui/AboutDialog.ui
    gui/ChangeMasterKeyWidget.ui
    gui/DatabaseOpenDialog.ui
    gui/DatabaseSettingsWidget.ui
    gui/EditWidget.ui
    gui/EditWidgetIcons.ui
    gui/MainWindow.ui
    gui/entry/EditEntryWidgetAdvanced.ui
    gui/entry/EditEntryWidgetHistory.ui
    gui/entry/EditEntryWidgetMain.ui
    gui/entry/EditEntryWidgetNotes.ui
    gui/group/EditGroupWidgetMain.ui
)

qt4_wrap_ui(keepassx_SOURCES ${keepassx_FORMS})
qt4_wrap_cpp(keepassx_SOURCES ${keepassx_MOC})

add_library( keepassx_core STATIC ${keepassx_SOURCES} )

add_executable( ${PROGNAME} WIN32 MACOSX_BUNDLE main.cpp )
target_link_libraries( ${PROGNAME} keepassx_core ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${GCRYPT_LIBRARIES} ${ZLIB_LIBRARIES} )
install(TARGETS ${PROGNAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${BIN_INSTALL_DIR} COMPONENT Runtime)
