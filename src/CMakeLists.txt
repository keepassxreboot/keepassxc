#  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

if(NOT GIT_HEAD OR NOT GIT_DESCRIBE)
    set(GIT_HEAD "")
    set(GIT_DESCRIBE "")
endif()

find_library(ZXCVBN_LIBRARIES zxcvbn)
if(NOT ZXCVBN_LIBRARIES)
    add_library(zxcvbn STATIC zxcvbn/zxcvbn.c)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/zxcvbn)
    set(ZXCVBN_LIBRARIES zxcvbn)
endif(NOT ZXCVBN_LIBRARIES)

configure_file(version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/version.h @ONLY)

set(keepassx_SOURCES
        core/AutoTypeAssociations.cpp
        core/AutoTypeMatch.cpp
        core/Compare.cpp
        core/Config.cpp
        core/CsvParser.cpp
        core/CustomData.cpp
        core/Database.cpp
        core/DatabaseIcons.cpp
        core/Entry.cpp
        core/EntryAttachments.cpp
        core/EntryAttributes.cpp
        core/EntrySearcher.cpp
        core/FilePath.cpp
        core/FileWatcher.cpp
        core/Bootstrap.cpp
        core/Group.cpp
        core/InactivityTimer.cpp
        core/Merger.cpp
        core/Metadata.cpp
        core/PasswordGenerator.cpp
        core/PassphraseGenerator.cpp
        core/SignalMultiplexer.cpp
        core/ScreenLockListener.cpp
        core/ScreenLockListenerPrivate.cpp
        core/TimeDelta.cpp
        core/TimeInfo.cpp
        core/Clock.cpp
        core/Tools.cpp
        core/Translator.cpp
        core/Base32.cpp
        cli/Utils.cpp
        cli/TextStream.cpp
        crypto/Crypto.cpp
        crypto/CryptoHash.cpp
        crypto/Random.cpp
        crypto/SymmetricCipher.cpp
        crypto/SymmetricCipherGcrypt.cpp
        crypto/kdf/Kdf.cpp
        crypto/kdf/AesKdf.cpp
        crypto/kdf/Argon2Kdf.cpp
        format/CsvExporter.cpp
        format/KeePass1Reader.cpp
        format/KeePass2.cpp
        format/KeePass2RandomStream.cpp
        format/KdbxReader.cpp
        format/KdbxWriter.cpp
        format/KdbxXmlReader.cpp
        format/KeePass2Reader.cpp
        format/KeePass2Writer.cpp
        format/Kdbx3Reader.cpp
        format/Kdbx3Writer.cpp
        format/Kdbx4Reader.cpp
        format/Kdbx4Writer.cpp
        format/KdbxXmlWriter.cpp
        gui/AboutDialog.cpp
        gui/Application.cpp
        gui/CategoryListWidget.cpp
        gui/Clipboard.cpp
        gui/CloneDialog.cpp
        gui/DatabaseOpenWidget.cpp
        gui/DatabaseTabWidget.cpp
        gui/DatabaseWidget.cpp
        gui/DatabaseWidgetStateSync.cpp
        gui/EntryPreviewWidget.cpp
        gui/DialogyWidget.cpp
        gui/DragTabBar.cpp
        gui/EditWidget.cpp
        gui/EditWidgetIcons.cpp
        gui/EditWidgetProperties.cpp
        gui/FileDialog.cpp
        gui/Font.cpp
        gui/IconModels.cpp
        gui/KeePass1OpenWidget.cpp
        gui/KMessageWidget.cpp
        gui/LineEdit.cpp
        gui/MainWindow.cpp
        gui/MessageBox.cpp
        gui/MessageWidget.cpp
        gui/PasswordEdit.cpp
        gui/PasswordGeneratorWidget.cpp
        gui/ApplicationSettingsWidget.cpp
        gui/SearchWidget.cpp
        gui/SortFilterHideProxyModel.cpp
        gui/SquareSvgWidget.cpp
        gui/TotpSetupDialog.cpp
        gui/TotpDialog.cpp
        gui/TotpExportSettingsDialog.cpp
        gui/DatabaseOpenDialog.cpp
        gui/WelcomeWidget.cpp
        gui/csvImport/CsvImportWidget.cpp
        gui/csvImport/CsvImportWizard.cpp
        gui/csvImport/CsvParserModel.cpp
        gui/entry/AutoTypeAssociationsModel.cpp
        gui/entry/AutoTypeMatchModel.cpp
        gui/entry/AutoTypeMatchView.cpp
        gui/entry/EditEntryWidget.cpp
        gui/entry/EntryAttachmentsModel.cpp
        gui/entry/EntryAttachmentsWidget.cpp
        gui/entry/EntryAttributesModel.cpp
        gui/entry/EntryHistoryModel.cpp
        gui/entry/EntryModel.cpp
        gui/entry/EntryView.cpp
        gui/group/EditGroupWidget.cpp
        gui/group/GroupModel.cpp
        gui/group/GroupView.cpp
        gui/masterkey/KeyComponentWidget.cpp
        gui/masterkey/PasswordEditWidget.cpp
        gui/masterkey/YubiKeyEditWidget.cpp
        gui/masterkey/KeyFileEditWidget.cpp
        gui/dbsettings/DatabaseSettingsWidget.cpp
        gui/dbsettings/DatabaseSettingsDialog.cpp
        gui/dbsettings/DatabaseSettingsWidgetGeneral.cpp
        gui/dbsettings/DatabaseSettingsWidgetMetaDataSimple.cpp
        gui/dbsettings/DatabaseSettingsWidgetEncryption.cpp
        gui/dbsettings/DatabaseSettingsWidgetMasterKey.cpp
        gui/settings/SettingsWidget.cpp
        gui/widgets/ElidedLabel.cpp
        gui/widgets/PopupHelpWidget.cpp
        gui/wizard/NewDatabaseWizard.cpp
        gui/wizard/NewDatabaseWizardPage.cpp
        gui/wizard/NewDatabaseWizardPageMetaData.cpp
        gui/wizard/NewDatabaseWizardPageEncryption.cpp
        gui/wizard/NewDatabaseWizardPageMasterKey.cpp
        keys/CompositeKey.cpp
        keys/FileKey.cpp
        keys/PasswordKey.cpp
        keys/YkChallengeResponseKey.cpp
        streams/HashedBlockStream.cpp
        streams/HmacBlockStream.cpp
        streams/LayeredStream.cpp
        streams/qtiocompressor.cpp
        streams/StoreDataStream.cpp
        streams/SymmetricCipherStream.cpp
        totp/totp.cpp)
if(APPLE)
    set(keepassx_SOURCES
            ${keepassx_SOURCES}
            core/ScreenLockListenerMac.cpp
            core/MacPasteboard.cpp
            gui/macutils/MacUtils.cpp
            gui/macutils/AppKitImpl.mm)
endif()
if(UNIX AND NOT APPLE)
    set(keepassx_SOURCES
            ${keepassx_SOURCES}
            core/ScreenLockListenerDBus.cpp
            gui/MainWindowAdaptor.cpp)
endif()
if(MINGW)
    set(keepassx_SOURCES
            ${keepassx_SOURCES}
            core/ScreenLockListenerWin.cpp)
endif()
if(MINGW OR (UNIX AND NOT APPLE))
    set(keepassx_SOURCES
            ${keepassx_SOURCES}
            core/OSEventFilter.cpp)
endif()

set(keepassx_SOURCES_MAINEXE main.cpp)

add_feature_info(Auto-Type WITH_XC_AUTOTYPE "Automatic password typing")
add_feature_info(Networking WITH_XC_NETWORKING "Compile KeePassXC with network access code (e.g. for downloading website icons)")
add_feature_info(KeePassXC-Browser WITH_XC_BROWSER "Browser integration with KeePassXC-Browser")
add_feature_info(SSHAgent WITH_XC_SSHAGENT "SSH agent integration compatible with KeeAgent")
add_feature_info(KeeShare WITH_XC_KEESHARE "Sharing integration with KeeShare")
add_feature_info(KeeShare-Secure WITH_XC_KEESHARE_SECURE "Sharing integration with KeeShare with secure sources")
add_feature_info(YubiKey WITH_XC_YUBIKEY "YubiKey HMAC-SHA1 challenge-response")
add_feature_info(Update-Check WITH_XC_UPDATECHECK "Check for updates on application startup")
if(APPLE)
    add_feature_info(TouchID WITH_XC_TOUCHID "TouchID integration")
endif()

set(BROWSER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/browser)
add_subdirectory(browser)
add_subdirectory(proxy)
if(WITH_XC_BROWSER)
    set(keepassxcbrowser_LIB keepassxcbrowser)
    set(keepassx_SOURCES ${keepassx_SOURCES} gui/dbsettings/DatabaseSettingsWidgetBrowser.cpp)
endif()

add_subdirectory(autotype)
add_subdirectory(cli)
add_subdirectory(qrcode)
set(qrcode_LIB qrcode)

add_subdirectory(crypto/ssh)
if(WITH_XC_CRYPTO_SSH)
    set(crypto_ssh_LIB crypto_ssh)
endif()

add_subdirectory(keeshare)
if(WITH_XC_KEESHARE)
    set(keeshare_LIB keeshare)
endif()

add_subdirectory(sshagent)
if(WITH_XC_SSHAGENT)
    set(sshagent_LIB sshagent)
endif()

set(autotype_SOURCES
        core/Tools.cpp
        autotype/AutoType.cpp
        autotype/AutoTypeAction.cpp
        autotype/AutoTypeSelectDialog.cpp
        autotype/AutoTypeSelectView.cpp
        autotype/ShortcutWidget.cpp
        autotype/WildcardMatcher.cpp
        autotype/WindowSelectComboBox.cpp)

if(MINGW)
    set(keepassx_SOURCES_MAINEXE ${keepassx_SOURCES_MAINEXE} ${CMAKE_SOURCE_DIR}/share/windows/icon.rc)
endif()

if(WITH_XC_YUBIKEY)
    list(APPEND keepassx_SOURCES keys/drivers/YubiKey.cpp)
else()
    list(APPEND keepassx_SOURCES keys/drivers/YubiKey.h keys/drivers/YubiKeyStub.cpp)
endif()

if(WITH_XC_UPDATECHECK AND WITH_XC_NETWORKING)
    list(APPEND keepassx_SOURCES updatecheck/UpdateCheck.cpp gui/UpdateCheckDialog.cpp)
endif()

if(WITH_XC_TOUCHID)
    list(APPEND keepassx_SOURCES touchid/TouchID.mm)
endif()

add_library(autotype STATIC ${autotype_SOURCES})
target_link_libraries(autotype Qt5::Core Qt5::Widgets)

add_library(keepassx_core STATIC ${keepassx_SOURCES})

set_target_properties(keepassx_core PROPERTIES COMPILE_DEFINITIONS KEEPASSX_BUILDING_CORE)
target_link_libraries(keepassx_core
        autotype
        ${keepassxcbrowser_LIB}
        ${qrcode_LIB}
        Qt5::Core
        Qt5::Concurrent
        Qt5::Network
        Qt5::Widgets
        ${YUBIKEY_LIBRARIES}
        ${ZXCVBN_LIBRARIES}
        ${ARGON2_LIBRARIES}
        ${GCRYPT_LIBRARIES}
        ${GPGERROR_LIBRARIES}
        ${ZLIB_LIBRARIES})

if(WITH_XC_SSHAGENT)
  target_link_libraries(keepassx_core sshagent)
endif()
if(WITH_XC_KEESHARE)
  target_link_libraries(keepassx_core keeshare)
endif()

if(APPLE)
    target_link_libraries(keepassx_core "-framework Foundation -framework AppKit")
    if(Qt5MacExtras_FOUND)
        target_link_libraries(keepassx_core Qt5::MacExtras)
    endif()
    if(WITH_XC_TOUCHID)
        target_link_libraries(keepassx_core "-framework Security")
        target_link_libraries(keepassx_core "-framework LocalAuthentication")
    endif()
endif()
if(UNIX AND NOT APPLE)
    target_link_libraries(keepassx_core Qt5::DBus)
endif()
if(MINGW)
    target_link_libraries(keepassx_core Wtsapi32.lib Ws2_32.lib)
endif()

if(MINGW)
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

add_executable(${PROGNAME} WIN32 ${keepassx_SOURCES_MAINEXE} ${WIN32_ProductVersionFiles})
target_link_libraries(${PROGNAME} keepassx_core)

set_target_properties(${PROGNAME} PROPERTIES ENABLE_EXPORTS ON)

if(APPLE AND WITH_APP_BUNDLE)
    configure_file(${CMAKE_SOURCE_DIR}/share/macosx/Info.plist.cmake ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
    set_target_properties(${PROGNAME} PROPERTIES
            MACOSX_BUNDLE ON
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)

    if(WITH_XC_TOUCHID)
        set_target_properties(${PROGNAME} PROPERTIES
                CPACK_BUNDLE_APPLE_ENTITLEMENTS "${CMAKE_SOURCE_DIR}/share/macosx/keepassxc.entitlements")
    endif()

    if(QT_MAC_USE_COCOA AND EXISTS "${QT_LIBRARY_DIR}/Resources/qt_menu.nib")
        install(DIRECTORY "${QT_LIBRARY_DIR}/Resources/qt_menu.nib"
                DESTINATION "${DATA_INSTALL_DIR}")
    endif()

    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_FORMAT "UDBZ")
    set(CPACK_DMG_DS_STORE "${CMAKE_SOURCE_DIR}/share/macosx/DS_Store.in")
    set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/share/macosx/background.tiff")
    set(CPACK_DMG_VOLUME_NAME "${PROGNAME}")
    set(CPACK_SYSTEM_NAME "OSX")
    set(CPACK_STRIP_FILES ON)
    set(CPACK_PACKAGE_FILE_NAME "${PROGNAME}-${KEEPASSXC_VERSION}")
    include(CPack)

    add_custom_command(TARGET ${PROGNAME}
            POST_BUILD
            COMMAND ${MACDEPLOYQT_EXE} ${PROGNAME}.app
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src
            COMMENT "Deploying app bundle")
endif()

install(TARGETS ${PROGNAME}
        BUNDLE DESTINATION . COMPONENT Runtime
        RUNTIME DESTINATION ${BIN_INSTALL_DIR} COMPONENT Runtime)

if(MINGW)
    if(${CMAKE_SIZEOF_VOID_P} EQUAL "8")
        set(OUTPUT_FILE_POSTFIX "Win64")
    else()
        set(OUTPUT_FILE_POSTFIX "Win32")
    endif()

    # We have to copy the license file in the configuration phase.
    # CMake checks that CPACK_RESOURCE_FILE_LICENSE actually exists and
    # we have to copy it because WiX needs it to have a .txt extension.
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/LICENSE.GPL-2"
            "${CMAKE_CURRENT_BINARY_DIR}/INSTALLER_LICENSE.txt")

    string(REGEX REPLACE "-snapshot$" "" KEEPASSXC_VERSION_CLEAN ${KEEPASSXC_VERSION})

    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_STRIP_FILES OFF)
    set(CPACK_PACKAGE_FILE_NAME "${PROGNAME}-${KEEPASSXC_VERSION}-${OUTPUT_FILE_POSTFIX}")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY ${PROGNAME})
    set(CPACK_PACKAGE_VERSION ${KEEPASSXC_VERSION_CLEAN})
    set(CPACK_PACKAGE_VENDOR "${PROGNAME} Team")
    string(REGEX REPLACE "/" "\\\\\\\\" CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/share/windows/installer-header.bmp")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_BINARY_DIR}/INSTALLER_LICENSE.txt")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/share/windows/keepassxc.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CPACK_NSIS_MUI_ICON}")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "\\\\${PROGNAME}.exe")
    string(REGEX REPLACE "/" "\\\\\\\\" CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CMAKE_SOURCE_DIR}/share/windows/installer-wizard.bmp")
    set(CPACK_NSIS_MUI_UNWELCOMEFINISHPAGE_BITMAP "${CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP}")
    set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${PROGNAME}.lnk' '$INSTDIR\\\\${PROGNAME}.exe'")
    set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$SMPROGRAMS\\\\$START_MENU\\\\${PROGNAME}.lnk'")
    set(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS "ExecWait 'Taskkill /IM KeePassXC.exe'")
    set(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS "${CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS}\nExecWait 'Taskkill /IM keepassxc-proxy.exe /F'")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "ExecWait 'Taskkill /IM KeePassXC.exe'")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS}\nExecWait 'Taskkill /IM keepassxc-proxy.exe /F'")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://keepassxc.org")
    set(CPACK_NSIS_DISPLAY_NAME ${PROGNAME})
    set(CPACK_NSIS_PACKAGE_NAME "${PROGNAME} v${KEEPASSXC_VERSION}")
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "../${PROGNAME}.exe")
    set(CPACK_WIX_UPGRADE_GUID 88785A72-3EAE-4F29-89E3-BC6B19BA9A5B)
    set(CPACK_WIX_PRODUCT_ICON "${CMAKE_SOURCE_DIR}/share/windows/keepassxc.ico")
    set(CPACK_WIX_UI_BANNER "${CMAKE_SOURCE_DIR}/share/windows/wix-banner.bmp")
    set(CPACK_WIX_UI_DIALOG "${CMAKE_SOURCE_DIR}/share/windows/wix-dialog.bmp")
    set(CPACK_WIX_TEMPLATE "${CMAKE_SOURCE_DIR}/share/windows/wix-template.xml")
    set(CPACK_WIX_PATCH_FILE "${CMAKE_SOURCE_DIR}/share/windows/wix-patch.xml")
    set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "https://keepassxc.org")
    set(CPACK_WIX_EXTENSIONS "WixUtilExtension.dll")
    include(CPack)

    install(CODE "
    set(gp_tool \"objdump\")
    " COMPONENT Runtime)

    include(DeployQt4)
    install_qt4_executable(${PROGNAME}.exe)

    # install Qt5 plugins
    set(PLUGINS_DIR ${Qt5_PREFIX}/share/qt5/plugins)
    install(FILES
            ${PLUGINS_DIR}/platforms/qwindows$<$<CONFIG:Debug>:d>.dll
            ${PLUGINS_DIR}/platforms/qdirect2d$<$<CONFIG:Debug>:d>.dll
            DESTINATION "platforms")
    install(FILES ${PLUGINS_DIR}/styles/qwindowsvistastyle$<$<CONFIG:Debug>:d>.dll DESTINATION "styles")
    install(FILES ${PLUGINS_DIR}/platforminputcontexts/qtvirtualkeyboardplugin$<$<CONFIG:Debug>:d>.dll DESTINATION "platforminputcontexts")
    install(FILES ${PLUGINS_DIR}/iconengines/qsvgicon$<$<CONFIG:Debug>:d>.dll DESTINATION "iconengines")
    install(FILES
            ${PLUGINS_DIR}/imageformats/qgif$<$<CONFIG:Debug>:d>.dll
            ${PLUGINS_DIR}/imageformats/qicns$<$<CONFIG:Debug>:d>.dll
            ${PLUGINS_DIR}/imageformats/qico$<$<CONFIG:Debug>:d>.dll
            ${PLUGINS_DIR}/imageformats/qjpeg$<$<CONFIG:Debug>:d>.dll
            ${PLUGINS_DIR}/imageformats/qwebp$<$<CONFIG:Debug>:d>.dll
            DESTINATION "imageformats")

    # install CA cert chains
    install(FILES ${Qt5_PREFIX}/ssl/certs/ca-bundle.crt DESTINATION "ssl/certs")
endif()
