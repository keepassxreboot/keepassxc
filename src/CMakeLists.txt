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

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

configure_file(config-keepassx.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-keepassx.h)

set(keepassx_SOURCES
    autotype/AutoType.cpp
    autotype/AutoTypeAction.cpp
    autotype/AutoTypePlatformPlugin.h
    autotype/AutoTypeSelectDialog.cpp
    autotype/AutoTypeSelectView.cpp
    autotype/ShortcutWidget.cpp
    autotype/WildcardMatcher.cpp
    autotype/WindowSelectComboBox.cpp
    autotype/test/AutoTypeTestInterface.h
    core/AutoTypeAssociations.cpp
    core/Config.cpp
    core/Database.cpp
    core/DatabaseIcons.cpp
    core/Endian.cpp
    core/Entry.cpp
    core/EntryAttachments.cpp
    core/EntryAttributes.cpp
    core/EntrySearcher.cpp
    core/FilePath.cpp
    core/Global.h
    core/Group.cpp
    core/InactivityTimer.cpp
    core/ListDeleter.h
    core/Metadata.cpp
    core/PasswordGenerator.cpp
    core/qsavefile.cpp
    core/qsavefile_p.h
    core/SignalMultiplexer.cpp
    core/TimeDelta.cpp
    core/TimeInfo.cpp
    core/ToDbExporter.cpp
    core/Tools.cpp
    core/Translator.cpp
    core/Uuid.cpp
    core/qcommandlineoption.cpp
    core/qcommandlineparser.cpp
    crypto/Crypto.cpp
    crypto/CryptoHash.cpp
    crypto/Random.cpp
    crypto/SymmetricCipher.cpp
    crypto/SymmetricCipherBackend.h
    crypto/SymmetricCipherGcrypt.cpp
    format/KeePass1.h
    format/KeePass1Reader.cpp
    format/KeePass2.h
    format/KeePass2RandomStream.cpp
    format/KeePass2Reader.cpp
    format/KeePass2Writer.cpp
    format/KeePass2XmlReader.cpp
    format/KeePass2XmlWriter.cpp
    gui/AboutDialog.cpp
    gui/Application.cpp
    gui/ChangeMasterKeyWidget.cpp
    gui/Clipboard.cpp
    gui/DatabaseOpenWidget.cpp
    gui/DatabaseSettingsWidget.cpp
    gui/DatabaseTabWidget.cpp
    gui/DatabaseWidget.cpp
    gui/DatabaseWidgetStateSync.cpp
    gui/DialogyWidget.cpp
    gui/DragTabBar.cpp
    gui/EditWidget.cpp
    gui/EditWidgetIcons.cpp
    gui/EditWidgetProperties.cpp
    gui/FileDialog.cpp
    gui/IconModels.cpp
    gui/KeePass1OpenWidget.cpp
    gui/LineEdit.cpp
    gui/MainWindow.cpp
    gui/MessageBox.cpp
    gui/PasswordEdit.cpp
    gui/PasswordGeneratorWidget.cpp
    gui/PasswordComboBox.cpp
    gui/SettingsWidget.cpp
    gui/SortFilterHideProxyModel.cpp
    gui/UnlockDatabaseWidget.cpp
    gui/WelcomeWidget.cpp
    gui/entry/AutoTypeAssociationsModel.cpp
    gui/entry/EditEntryWidget.cpp
    gui/entry/EditEntryWidget_p.h
    gui/entry/EntryAttachmentsModel.cpp
    gui/entry/EntryAttributesModel.cpp
    gui/entry/EntryHistoryModel.cpp
    gui/entry/EntryModel.cpp
    gui/entry/EntryView.cpp
    gui/group/EditGroupWidget.cpp
    gui/group/GroupModel.cpp
    gui/group/GroupView.cpp
    keys/CompositeKey.cpp
    keys/CompositeKey_p.h
    keys/FileKey.cpp
    keys/Key.h
    keys/PasswordKey.cpp
    streams/HashedBlockStream.cpp
    streams/LayeredStream.cpp
    streams/qtiocompressor.cpp
    streams/StoreDataStream.cpp
    streams/SymmetricCipherStream.cpp
)

if(NOT GCRYPT_HAS_SALSA20)
  set(keepassx_SOURCES
      ${keepassx_SOURCES}
      crypto/salsa20/ecrypt-config.h
      crypto/salsa20/ecrypt-machine.h
      crypto/salsa20/ecrypt-portable.h
      crypto/salsa20/ecrypt-sync.h
      crypto/salsa20/salsa20.c
      crypto/SymmetricCipherSalsa20.cpp
  )
endif()

set(keepassx_SOURCES_MAINEXE
    main.cpp
)

set(keepassx_MOC
    autotype/AutoType.h
    autotype/AutoTypeSelectDialog.h
    autotype/AutoTypeSelectView.h
    autotype/ShortcutWidget.h
    autotype/WindowSelectComboBox.h
    core/AutoTypeAssociations.h
    core/Config.h
    core/Database.h
    core/Entry.h
    core/EntryAttachments.h
    core/EntryAttributes.h
    core/Group.h
    core/InactivityTimer.h
    core/Metadata.h
    core/qsavefile.h
    gui/AboutDialog.h
    gui/Application.h
    gui/ChangeMasterKeyWidget.h
    gui/Clipboard.h
    gui/DatabaseOpenWidget.h
    gui/DatabaseSettingsWidget.h
    gui/DatabaseTabWidget.h
    gui/DatabaseWidget.h
    gui/DatabaseWidgetStateSync.h
    gui/DialogyWidget.h
    gui/DragTabBar.h
    gui/EditWidget.h
    gui/EditWidgetIcons.h
    gui/EditWidgetProperties.h
    gui/IconModels.h
    gui/KeePass1OpenWidget.h
    gui/LineEdit.h
    gui/MainWindow.h
    gui/PasswordEdit.h
    gui/PasswordGeneratorWidget.h
    gui/PasswordComboBox.h
    gui/SettingsWidget.h
    gui/SortFilterHideProxyModel.h
    gui/UnlockDatabaseWidget.h
    gui/WelcomeWidget.h
    gui/entry/AutoTypeAssociationsModel.h
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
    streams/StoreDataStream.h
    streams/SymmetricCipherStream.h
)

set(keepassx_FORMS
    gui/AboutDialog.ui
    gui/ChangeMasterKeyWidget.ui
    gui/DatabaseOpenWidget.ui
    gui/DatabaseSettingsWidget.ui
    gui/EditWidget.ui
    gui/EditWidgetIcons.ui
    gui/EditWidgetProperties.ui
    gui/MainWindow.ui
    gui/PasswordGeneratorWidget.ui
    gui/SearchWidget.ui
    gui/SettingsWidgetGeneral.ui
    gui/SettingsWidgetSecurity.ui
    gui/WelcomeWidget.ui
    gui/entry/EditEntryWidgetAdvanced.ui
    gui/entry/EditEntryWidgetAutoType.ui
    gui/entry/EditEntryWidgetHistory.ui
    gui/entry/EditEntryWidgetMain.ui
    gui/group/EditGroupWidgetMain.ui
)

if(MINGW)
  set(keepassx_SOURCES_MAINEXE
      ${keepassx_SOURCES_MAINEXE}
      ${CMAKE_SOURCE_DIR}/share/windows/icon.rc)
endif()

qt4_wrap_ui(keepassx_SOURCES ${keepassx_FORMS})
qt4_wrap_cpp(keepassx_SOURCES ${keepassx_MOC})

add_library(keepassx_core STATIC ${keepassx_SOURCES})
set_target_properties(keepassx_core PROPERTIES COMPILE_DEFINITIONS KEEPASSX_BUILDING_CORE)

add_executable(${PROGNAME} WIN32 MACOSX_BUNDLE ${keepassx_SOURCES_MAINEXE})
target_link_libraries(${PROGNAME}
                      keepassx_core
                      ${QT_QTCORE_LIBRARY}
                      ${QT_QTGUI_LIBRARY}
                      ${GCRYPT_LIBRARIES}
                      ${ZLIB_LIBRARIES})

if(UNIX AND NOT APPLE)
  target_link_libraries(${PROGNAME} ${QT_QTDBUS_LIBRARY})
endif()

set_target_properties(${PROGNAME} PROPERTIES ENABLE_EXPORTS ON)

if(APPLE)
  configure_file(${CMAKE_SOURCE_DIR}/share/macosx/Info.plist.cmake ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
  set_target_properties(${PROGNAME} PROPERTIES
                        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
endif()

install(TARGETS ${PROGNAME}
        BUNDLE DESTINATION . COMPONENT Runtime
        RUNTIME DESTINATION ${BIN_INSTALL_DIR} COMPONENT Runtime)

add_subdirectory(autotype)

if(APPLE AND NOT (${CMAKE_VERSION} VERSION_LESS 2.8.8))
  if(QT_MAC_USE_COCOA AND EXISTS "${QT_LIBRARY_DIR}/Resources/qt_menu.nib")
    install(DIRECTORY "${QT_LIBRARY_DIR}/Resources/qt_menu.nib"
            DESTINATION "${DATA_INSTALL_DIR}")
  endif()

  set(CPACK_GENERATOR "DragNDrop")
  set(CPACK_DMG_FORMAT "UDBZ")
  set(CPACK_DMG_VOLUME_NAME "${PROGNAME}")
  set(CPACK_SYSTEM_NAME "OSX")
  set(CPACK_STRIP_FILES ON)
  set(CPACK_PACKAGE_FILE_NAME "${PROGNAME}-${KEEPASSX_VERSION_NUM}")
  include(CPack)

  include(DeployQt4)
  install_qt4_executable(${PROGNAME}.app "qjpeg;qgif;qico;qtaccessiblewidgets")
endif()

if(MINGW AND NOT (${CMAKE_VERSION} VERSION_LESS 2.8.8))
  set(CPACK_GENERATOR "ZIP")
  set(CPACK_STRIP_FILES ON)
  set(CPACK_PACKAGE_FILE_NAME "${PROGNAME}-${KEEPASSX_VERSION_NUM}")
  include(CPack)

  include(DeployQt4)
  install_qt4_executable(${PROGNAME}.exe "qjpeg;qgif;qico;qtaccessiblewidgets")
endif()
