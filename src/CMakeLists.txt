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

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_HEAD)
git_describe(GIT_DESCRIBE --long)

if (NOT GIT_HEAD OR NOT GIT_DESCRIBE)
    set(GIT_HEAD "")
    set(GIT_DESCRIBE "")
endif()

configure_file(version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/version.h @ONLY)

set(keepassx_SOURCES
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
    core/SignalMultiplexer.cpp
    core/TimeDelta.cpp
    core/TimeInfo.cpp
    core/ToDbExporter.cpp
    core/Tools.cpp
    core/Translator.cpp
    core/Uuid.cpp
    crypto/Crypto.cpp
    crypto/CryptoHash.cpp
    crypto/Random.cpp
    crypto/SymmetricCipher.cpp
    crypto/SymmetricCipherBackend.h
    crypto/SymmetricCipherGcrypt.cpp
    format/CsvExporter.cpp
    format/KeePass1.h
    format/KeePass1Reader.cpp
    format/KeePass2.h
    format/KeePass2RandomStream.cpp
    format/KeePass2Reader.cpp
    format/KeePass2Repair.cpp
    format/KeePass2Writer.cpp
    format/KeePass2XmlReader.cpp
    format/KeePass2XmlWriter.cpp
    gui/AboutDialog.cpp
    gui/Application.cpp
    gui/ChangeMasterKeyWidget.cpp
    gui/Clipboard.cpp
    gui/DatabaseOpenWidget.cpp
    gui/DatabaseRepairWidget.cpp
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
    gui/KMessageWidget.cpp
    gui/LineEdit.cpp
    gui/MainWindow.cpp
    gui/MessageBox.cpp
    gui/MessageWidget.cpp
    gui/PasswordEdit.cpp
    gui/PasswordGeneratorWidget.cpp
    gui/PasswordComboBox.cpp
    gui/SettingsWidget.cpp
    gui/SearchWidget.cpp
    gui/SortFilterHideProxyModel.cpp
    gui/UnlockDatabaseWidget.cpp
    gui/UnlockDatabaseDialog.cpp
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

set(keepassx_SOURCES_MAINEXE
    main.cpp
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

add_feature_info(KeePassHTTP WITH_XC_HTTP "KeePassHTTP support for ChromeIPass and PassIFox")
add_feature_info(Autotype WITH_XC_AUTOTYPE "Auto-type passwords in Input fields")

add_subdirectory(http)
if(WITH_XC_HTTP)
    set(keepasshttp_LIB keepasshttp)
endif()

add_subdirectory(autotype)

set(autotype_SOURCES
    core/Tools.cpp
    autotype/AutoType.cpp
    autotype/AutoTypeAction.cpp
    autotype/AutoTypePlatformPlugin.h
    autotype/AutoTypeSelectDialog.cpp
    autotype/AutoTypeSelectView.cpp
    autotype/ShortcutWidget.cpp
    autotype/WildcardMatcher.cpp
    autotype/WindowSelectComboBox.cpp
    autotype/test/AutoTypeTestInterface.h
)

if(MINGW)
  set(keepassx_SOURCES_MAINEXE
      ${keepassx_SOURCES_MAINEXE}
      ${CMAKE_SOURCE_DIR}/share/windows/icon.rc)
endif()

qt5_wrap_ui(keepassx_SOURCES ${keepassx_FORMS})

add_library(zxcvbn STATIC zxcvbn/zxcvbn.cpp)
target_link_libraries(zxcvbn)

add_library(autotype STATIC ${autotype_SOURCES})
target_link_libraries(autotype Qt5::Core Qt5::Widgets)

set(autotype_LIB autotype)

add_library(keepassx_core STATIC ${keepassx_SOURCES})

set_target_properties(keepassx_core PROPERTIES COMPILE_DEFINITIONS KEEPASSX_BUILDING_CORE)
target_link_libraries(keepassx_core
                      ${keepasshttp_LIB}
                      ${autotype_LIB}
                      zxcvbn
                      qhttp
                      Qt5::Core
                      Qt5::Concurrent
                      Qt5::Widgets
                      Qt5::Network
                      ${GCRYPT_LIBRARIES}
                      ${GPGERROR_LIBRARIES}
                      ${ZLIB_LIBRARIES})
if (UNIX AND NOT APPLE)
    target_link_libraries(keepassx_core Qt5::DBus)
endif()

if(MINGW)
  string(REPLACE "." ";" VERSION_LIST ${KEEPASSXC_VERSION})
  list(GET VERSION_LIST 0 KEEPASSXC_VERSION_MAJOR)
  list(GET VERSION_LIST 1 KEEPASSXC_VERSION_MINOR)
  list(GET VERSION_LIST 2 KEEPASSXC_VERSION_PATCH)

  include(GenerateProductVersion)
  generate_product_version(
    WIN32_ProductVersionFiles
    NAME "KeePassXC"
    COMPANY_NAME "KeePassXC Team"
    VERSION_MAJOR ${KEEPASSXC_VERSION_MAJOR}
    VERSION_MINOR ${KEEPASSXC_VERSION_MINOR}
    VERSION_PATCH ${KEEPASSXC_VERSION_PATCH}
  )
endif()

add_executable(${PROGNAME} WIN32 MACOSX_BUNDLE ${keepassx_SOURCES_MAINEXE} ${WIN32_ProductVersionFiles})
target_link_libraries(${PROGNAME} keepassx_core)

set_target_properties(${PROGNAME} PROPERTIES ENABLE_EXPORTS ON)

if(APPLE)
  configure_file(${CMAKE_SOURCE_DIR}/share/macosx/Info.plist.cmake ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
  set_target_properties(${PROGNAME} PROPERTIES
                        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
endif()

install(TARGETS ${PROGNAME}
        BUNDLE DESTINATION . COMPONENT Runtime
        RUNTIME DESTINATION ${BIN_INSTALL_DIR} COMPONENT Runtime)

if(APPLE)
  if(QT_MAC_USE_COCOA AND EXISTS "${QT_LIBRARY_DIR}/Resources/qt_menu.nib")
    install(DIRECTORY "${QT_LIBRARY_DIR}/Resources/qt_menu.nib"
            DESTINATION "${DATA_INSTALL_DIR}")
  endif()

  set(CPACK_GENERATOR "DragNDrop")
  set(CPACK_DMG_FORMAT "UDBZ")
  set(CPACK_DMG_VOLUME_NAME "${PROGNAME}")
  set(CPACK_SYSTEM_NAME "OSX")
  set(CPACK_STRIP_FILES ON)
  set(CPACK_PACKAGE_FILE_NAME "${PROGNAME}-${KEEPASSXC_VERSION_NUM}")
  include(CPack)

  if(NOT DEFINED QT_BINARY_DIR)
    set(QT_BINARY_DIR "/usr/local/opt/qt5/bin" CACHE PATH "QT binary folder")
  endif()
  add_custom_command(TARGET ${PROGNAME}
                     POST_BUILD
                     COMMAND ${QT_BINARY_DIR}/macdeployqt ${PROGNAME}.app
                     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src
                     COMMENT "Deploying app bundle")
endif()

if(MINGW)
  string(REPLACE "AMD" "Win" OUTPUT_FILE_POSTFIX "${CMAKE_HOST_SYSTEM_PROCESSOR}")
  set(CPACK_GENERATOR "ZIP;NSIS")
  set(CPACK_STRIP_FILES ON)
  set(CPACK_PACKAGE_FILE_NAME "${PROGNAME}-${KEEPASSXC_VERSION}-${OUTPUT_FILE_POSTFIX}")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY ${PROGNAME})
  set(CPACK_PACKAGE_VERSION ${KEEPASSXC_VERSION})
  set(CPACK_PACKAGE_VENDOR "${PROGNAME} Team")
  string(REGEX REPLACE "/" "\\\\\\\\" CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/share/windows/installer-header.bmp")
  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.GPL-2")
  set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/share/windows/keepassxc.ico")
  set(CPACK_NSIS_MUI_UNIICON "${CPACK_NSIS_MUI_ICON}")
  set(CPACK_NSIS_INSTALLED_ICON_NAME "\\\\${PROGNAME}.exe")
  string(REGEX REPLACE "/" "\\\\\\\\" CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CMAKE_SOURCE_DIR}/share/windows/installer-wizard.bmp")
  set(CPACK_NSIS_MUI_UNWELCOMEFINISHPAGE_BITMAP "${CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP}")
  set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${PROGNAME}.lnk' '$INSTDIR\\\\${PROGNAME}.exe'")
  set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$SMPROGRAMS\\\\$START_MENU\\\\${PROGNAME}.lnk'")
  set(CPACK_NSIS_URL_INFO_ABOUT "https://keepassxc.org")
  set(CPACK_NSIS_PACKAGE_NAME "${PROGNAME} v${KEEPASSXC_VERSION}")
  set(CPACK_NSIS_MUI_FINISHPAGE_RUN "../${PROGNAME}.exe")
  include(CPack)

  install(CODE "
  set(gp_tool \"objdump\")
  " COMPONENT Runtime)

  include(DeployQt4)
  install_qt4_executable(${PROGNAME}.exe)
  add_custom_command(TARGET ${PROGNAME} POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different ${Qt5Core_DIR}/../../../share/qt5/plugins/platforms/qwindows$<$<CONFIG:Debug>:d>.dll
                     $<TARGET_FILE_DIR:${PROGNAME}>)
  install(FILES $<TARGET_FILE_DIR:${PROGNAME}>/qwindows$<$<CONFIG:Debug>:d>.dll DESTINATION "platforms")
endif()
