# KeePassXC - Cross-Platform Password Manager
# Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
# Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.14)

# define project version
set(KEEPASSXC_VERSION_MAJOR "2")
set(KEEPASSXC_VERSION_MINOR "8")
set(KEEPASSXC_VERSION_PATCH "0")
set(KEEPASSXC_VERSION "${KEEPASSXC_VERSION_MAJOR}.${KEEPASSXC_VERSION_MINOR}.${KEEPASSXC_VERSION_PATCH}")
set(KEEPASSXC_VERSION_FULL "${KEEPASSXC_VERSION}")

set(ENV{CLICOLOR_FORCE} 1)

project(KeePassXC VERSION ${KEEPASSXC_VERSION} LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_VERSION VERSION_GREATER "3.18")
    cmake_policy(SET CMP0100 NEW)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GenerateProductVersion)
include(CompilerFlags)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    set(CMAKE_INSTALL_MANDIR "${CMAKE_INSTALL_FULL_MANDIR}")
    set(CMAKE_INSTALL_APPDATADIR "${CMAKE_INSTALL_FULL_DATADIR}/metainfo")
    set(CMAKE_INSTALL_APPDIR "${CMAKE_INSTALL_FULL_DATADIR}/applications")
    set(CMAKE_INSTALL_ICONDIR "${CMAKE_INSTALL_FULL_DATADIR}/icons")
    set(CMAKE_INSTALL_SOUNDDIR "${CMAKE_INSTALL_FULL_DATADIR}/sounds")
    set(CMAKE_INSTALL_XDG_CONFIGDIR "${CMAKE_INSTALL_FULL_SYSCONFDIR}/xdg")
    set(CMAKE_INSTALL_XDG_DESKTOPDIR "${CMAKE_INSTALL_FULL_DATADIR}/desktop-directories")
endif()

# Prefer OpenSSL on all platforms
if(APPLE)
    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl;/opt/homebrew/opt/openssl")
endif()

if(MINGW)
    set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")
endif()

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets Network Svg Test Concurrent)
find_package(Qt5 COMPONENTS WaylandClient QUIET)
find_package(OpenSSL 1.1.1 REQUIRED)
find_package(ZLIB REQUIRED)

if(NOT WIN32 AND NOT APPLE)
    find_package(Qt5 COMPONENTS DBus QUIET)
    find_package(X11 QUIET)
    if(NOT X11_FOUND AND NOT Qt5WaylandClient_FOUND)
        message(FATAL_ERROR "Either X11 or Wayland support is required.")
    endif()
endif()

option(WITH_TESTS "Enable building of unit tests" OFF)
option(WITH_XC_AUTOTYPE "Include Autotype." ON)
option(WITH_XC_BROWSER "Include Browser Integration." ON)
option(WITH_XC_NETWORKING "Include Networking (KeeShare)." ON)
option(WITH_XC_SSHAGENT "Include SSH Agent." ON)
option(WITH_XC_KEESHARE "Include KeeShare." ON)
option(WITH_XC_FDOSECRETS "Implement freedesktop.org Secret Storage spec." ON)
option(WITH_XC_YUBIKEY "Include YubiKey support." ON)
option(WITH_XC_ALL "Include all above features" OFF)

if(WITH_XC_ALL)
    set(WITH_XC_AUTOTYPE ON)
    set(WITH_XC_BROWSER ON)
    set(WITH_XC_NETWORKING ON)
    set(WITH_XC_SSHAGENT ON)
    set(WITH_XC_KEESHARE ON)
    set(WITH_XC_FDOSECRETS ON)
    set(WITH_XC_YUBIKEY ON)
endif()

if(WITH_XC_KEESHARE)
    set(WITH_XC_NETWORKING ON)
endif()

# check for libcurl with SFTP support
if(WITH_XC_NETWORKING)
    find_package(CURL 7.32.0 REQUIRED)
    # Check for SFTP support
    include(CheckSymbolExists)
    check_symbol_exists(CURL_VERSION_SSH "curl/curl.h" HAVE_CURL_SFTP)
    if(NOT HAVE_CURL_SFTP)
        message(WARNING "libcurl was built without SSH/SFTP support. KeeShare will not support SFTP.")
    endif()
endif()

# X11 and other Linux specific libraries
if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(libsecret IMPORTED_TARGET libsecret-1)
    pkg_check_modules(Qt5X11Extras IMPORTED_TARGET Qt5X11Extras QUIET)

    if(WITH_XC_AUTOTYPE)
        find_package(XCB COMPONENTS XCB UTIL REQUIRED)
        if(Qt5X11Extras_FOUND)
            find_package(Qt5 COMPONENTS X11Extras QUIET)
        endif()
    endif()

    if(WITH_XC_YUBIKEY)
        pkg_check_modules(yubikey yubikey-personalization QUIET)
        pkg_check_modules(yubikeypers yubikey-personalization QUIET)
    endif()

    find_package(Readline)
endif()

if(WIN32)
    if(MINGW)
        # MinGW-specific configurations
        add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7 and later
        add_definitions(-DWINVER=0x0601)
        add_definitions(-D_WIN32_IE=0x0700)
        
        # Enable MinGW compatibility features
        add_definitions(-DMINGW_HAS_SECURE_API=1)
        add_definitions(-D__USE_MINGW_ANSI_STDIO=1)
        
        # Include MinGW-specific source files
        set(MINGW_COMPAT_SOURCES
            src/core/compat/mingw_compat.cpp
        )
        
        if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/core/compat/mingw_compat.cpp")
            message(WARNING "MinGW compatibility file not found: src/core/compat/mingw_compat.cpp")
        endif()
    endif()
    
    # Windows-specific configurations
    find_package(Qt5 COMPONENTS WinExtras QUIET)
    add_definitions(-DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_ATL_SECURE_NO_WARNINGS)
    
    # Enable secure API
    add_definitions(-DSTRSAFE_NO_DEPRECATE)
endif()

if(UNIX)
    set(CMAKE_AUTOMOC_MOC_OPTIONS "-DNOWM_SILENTLY -DQT_NO_FOREACH -DQT_NO_JAVA_STYLE_ITERATORS")
else()
    set(CMAKE_AUTOMOC_MOC_OPTIONS "-DQT_NO_FOREACH -DQT_NO_JAVA_STYLE_ITERATORS")
endif()

if(WITH_XC_YUBIKEY)
    if(NOT APPLE AND NOT WIN32)
        if(NOT DEFINED ENV{YKPERS_STATIC})
            set(YKPERS_STATIC OFF CACHE BOOL "Link yubikey-personalization statically")
        else()
            set(YKPERS_STATIC $ENV{YKPERS_STATIC})
        endif()
    endif()
endif()

if(WITH_XC_BROWSER)
    if(WIN32 OR APPLE)
        find_package(Qt5 COMPONENTS Svg QUIET)
    else()
        find_package(Qt5 COMPONENTS Svg REQUIRED)
    endif()
endif()

if(WITH_XC_BROWSER OR WITH_XC_NETWORKING)
    find_package(Qt5 COMPONENTS Svg QUIET)
endif()

if(APPLE)
    find_package(Qt5 COMPONENTS MacExtras QUIET)
    find_package(Qt5 COMPONENTS DBus QUIET)
    # Detect if we are building on macOS 10.14+ to set new dark mode support
    execute_process(COMMAND sw_vers -productVersion
        OUTPUT_VARIABLE MACOS_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX MATCH "^10\\.([0-9]+)" _ "${MACOS_VERSION}")
    if(CMAKE_MATCH_1 GREATER 13 OR MACOS_VERSION VERSION_GREATER_EQUAL "11")
        set(APPLE_NEW_DARK_MODE ON)
    endif()
endif()

if(WITH_XC_AUTOTYPE)
    if(APPLE)
        find_library(CARBON_LIBRARY Carbon)
        find_library(COREFOUNDATION_LIBRARY CoreFoundation)
        find_library(SECURITY_LIBRARY Security)
        find_library(COCOA_LIBRARY Cocoa)
        mark_as_advanced(CARBON_LIBRARY COREFOUNDATION_LIBRARY SECURITY_LIBRARY COCOA_LIBRARY)
    endif()
endif()

add_definitions(
    -DQT_NO_URL_CAST_FROM_STRING
    -DQT_NO_FOREACH
    -DQT_NO_JAVA_STYLE_ITERATORS
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_USE_QSTRINGBUILDER
)

if(WITH_XC_FDOSECRETS)
    add_definitions(-DWITH_XC_FDOSECRETS)
endif()

if(WITH_XC_NETWORKING)
    add_definitions(-DWITH_XC_NETWORKING)
    if(HAVE_CURL_SFTP)
        add_definitions(-DHAVE_CURL_SFTP)
    endif()
endif()

if(WITH_XC_BROWSER)
    add_definitions(-DWITH_XC_BROWSER)
endif()

if(WITH_XC_YUBIKEY)
    add_definitions(-DWITH_XC_YUBIKEY)
endif()

if(APPLE AND WITH_XC_TOUCHID)
    add_definitions(-DWITH_XC_TOUCHID)
endif()

if(APPLE AND WITH_XC_APPLESCRIPT)
    add_definitions(-DWITH_XC_APPLESCRIPT)
endif()

if(APPLE AND WITH_XC_KEESHARE)
    add_definitions(-DWITH_XC_KEESHARE)
endif()

if(NOT APPLE)
    add_definitions(-DWITH_XC_KEESHARE)
endif()

if(WITH_XC_SSHAGENT)
    add_definitions(-DWITH_XC_SSHAGENT)
endif()

if(WITH_XC_AUTOTYPE)
    add_definitions(-DWITH_XC_AUTOTYPE)
    if(APPLE)
        add_definitions(-DWITH_XC_AUTOTYPE_MAC)
    endif()
    if(UNIX AND NOT APPLE)
        add_definitions(-DWITH_XC_AUTOTYPE_XCB)
    endif()
    if(WIN32)
        add_definitions(-DWITH_XC_AUTOTYPE_WINDOWS)
    endif()
endif()

# Source files
set(keepassx_SOURCES
    src/core/Alloc.cpp
    src/core/AutoTypeAssociations.cpp
    src/core/AutoTypeMatch.cpp
    src/core/AutoType.cpp
    src/core/Bootstrap.cpp
    src/core/Clock.cpp
    src/core/CsvParser.cpp
    src/core/CustomData.cpp
    src/core/Database.cpp
    src/core/DatabaseIcons.cpp
    src/core/DatabaseManager.cpp
    src/core/DatabaseWidget.cpp
    src/core/Entry.cpp
    src/core/EntryAttributes.cpp
    src/core/EntrySearcher.cpp
    src/core/EntryAttachments.cpp
    src/core/EntryHistory.cpp
    src/core/Global.cpp
    src/core/Group.cpp
    src/core/Merger.cpp
    src/core/Metadata.cpp
    src/core/PasswordGenerator.cpp
    src/core/PasswordHealth.cpp
    src/core/qrcode/UriParser.cpp
    src/core/RemoteDatabaseManager.cpp
    src/core/Resources.cpp
    src/core/SignalT.h
    src/core/ShortcutSettings.cpp
    src/core/Tools.cpp
    src/core/Translator.cpp
    src/core/Uuid.cpp
    src/core/qrcode/QrCode.cpp
    src/streams/HkdfStream.cpp
    src/streams/HashedBlockStream.cpp
    src/streams/StoreDataStream.cpp
    src/streams/qCompressDevice.cpp
    src/streams/LayeredStream.cpp
    src/streams/SymmetricCipherStream.cpp
)

# Add MinGW compatibility sources if needed
if(WIN32 AND MINGW)
    list(APPEND keepassx_SOURCES ${MINGW_COMPAT_SOURCES})
endif()

set(keepassx_FORMS
    src/AboutDialog.ui
    src/EntryPreviewWidget.ui
    src/MainWindow.ui
    src/SearchWidget.ui
    src/gui/AboutDialog.ui
    src/gui/EntryPreviewWidget.ui
    src/gui/MainWindow.ui
    src/gui/SearchWidget.ui
    src/gui/csvImport/CsvImportWidget.ui
    src/gui/dbsettings/DatabaseSettingsWidgetBrowser.ui
    src/gui/dbsettings/DatabaseSettingsWidgetKeeShare.ui
    src/gui/dbsettings/DatabaseSettingsWidgetMasterKey.ui
    src/gui/dbsettings/DatabaseSettingsWidgetRemote.ui
    src/gui/dbsettings/DatabaseSettingsWidgetStatistics.ui
    src/gui/entry/EditEntryWidget.ui
    src/gui/entry/EditEntryWidgetAdvanced.ui
    src/gui/entry/EditEntryWidgetAutoType.ui
    src/gui/entry/EditEntryWidgetBrowser.ui
    src/gui/entry/EditEntryWidgetHistory.ui
    src/gui/entry/EditEntryWidgetIcons.ui
    src/gui/entry/EditEntryWidgetMain.ui
    src/gui/entry/EditEntryWidgetSSHAgent.ui
    src/gui/group/EditGroupWidget.ui
    src/gui/group/EditGroupWidgetEntries.ui
    src/gui/group/EditGroupWidgetMain.ui
    src/gui/group/EditGroupWidgetNotes.ui
    src/gui/group/EditGroupWidgetSearch.ui
    src/gui/wizard/NewDatabaseWizardPageMetaData.ui
    src/gui/wizard/NewDatabaseWizardPageEncryption.ui
    src/gui/wizard/NewDatabaseWizardPageMasterKey.ui
    src/gui/wizard/NewDatabaseWizardPageDatabaseInfo.ui
    src/gui/wizard/NewDatabaseWizardPageFinal.ui
)

if(WITH_XC_AUTOTYPE)
    list(APPEND keepassx_SOURCES
        src/autotype/AutoType.cpp
        src/autotype/AutoTypeSelectDialog.cpp
        src/autotype/AutoTypeSelectView.cpp
        src/autotype/AutoTypeSelectWidget.cpp
    )
endif()

if(WITH_XC_BROWSER)
    list(APPEND keepassx_SOURCES
        src/browser/BrowserAction.cpp
        src/browser/BrowserClient.cpp
        src/browser/BrowserEntryConfig.cpp
        src/browser/BrowserEntrySaveDialog.cpp
        src/browser/BrowserHost.cpp
        src/browser/BrowserMessageBuilder.cpp
        src/browser/BrowserService.cpp
        src/browser/BrowserShared.cpp
        src/browser/NativeBrowserInstaller.cpp
        src/browser/NativeMessagingBase.cpp
        src/browser/NativeMessagingHost.cpp
        src/browser/NativeMessagingWin.cpp
    )
endif()

if(WITH_XC_NETWORKING)
    list(APPEND keepassx_SOURCES
        src/keeshare/KeeShare.cpp
        src/keeshare/KeeShareSettings.cpp
        src/keeshare/KeeShareSettingsImport.cpp
        src/keeshare/KeeShareSettingsImportExport.cpp
        src/keeshare/KeeShareSettingsExport.cpp
        src/keeshare/KeeShareSettingsSigning.cpp
        src/keeshare/KeeShareSettingsSync.cpp
    )
endif()

if(WITH_XC_SSHAGENT)
    list(APPEND keepassx_SOURCES
        src/sshagent/ASN1Key.cpp
        src/sshagent/AgentSettingsWidget.ui
        src/sshagent/AgentSettingsWidget.cpp
        src/sshagent/BinaryStream.cpp
        src/sshagent/KeeAgentSettings.cpp
        src/sshagent/SSHAgent.cpp
        src/sshagent/SSHAgentForwarding.cpp
    )
endif()

if(WITH_XC_FDOSECRETS)
    list(APPEND keepassx_SOURCES
        src/fdosecrets/FdoSecrets.cpp
        src/fdosecrets/FdoSecretsPlugin.cpp
        src/fdosecrets/objects/Collection.cpp
        src/fdosecrets/objects/Item.cpp
        src/fdosecrets/objects/Service.cpp
        src/fdosecrets/objects/Session.cpp
    )
endif()

if(WITH_XC_YUBIKEY)
    list(APPEND keepassx_SOURCES
        src/keys/drivers/YubiKey.cpp
        src/keys/drivers/YubiKeyInterface.cpp
        src/keys/drivers/YubiKeyInterfaceChalResp.cpp
        src/keys/drivers/YubiKeyInterfacePCSC.cpp
    )
endif()

# Platform specific sources
if(WIN32)
    list(APPEND keepassx_SOURCES
        src/winhello/WinHello.cpp
        src/winhello/WinHelloPlugin.cpp
    )
    
    if(MINGW)
        # Add MinGW-specific implementations
        list(APPEND keepassx_SOURCES
            src/core/compat/mingw_compat.cpp
        )
    endif()
endif()

if(APPLE)
    list(APPEND keepassx_SOURCES
        src/touchid/TouchID.mm
        src/macutils/AppKitInterface.mm
        src/macutils/MacGlobal.cpp
    )
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(WITH_XC_NETWORKING AND CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIRS})
endif()

if(Qt5Svg_FOUND)
    include_directories(${Qt5Svg_INCLUDE_DIRS})
endif()

# Create the main application executable
add_executable(${PROGNAME}
    src/main.cpp
    ${keepassx_SOURCES}
)

if(WIN32 AND MINGW)
    # Link MinGW-specific libraries
    target_link_libraries(${PROGNAME} PRIVATE
        ${QT_LIBRARIES}
        OpenSSL::SSL
        OpenSSL::Crypto
        ZLIB::ZLIB
        ${CURL_LIBRARIES}
        ${YKPERS_LIBRARIES}
        winmm
        ws2_32
        shlwapi
        crypt32
        ole32
        uuid
    )
else()
    target_link_libraries(${PROGNAME} PRIVATE
        ${QT_LIBRARIES}
        OpenSSL::SSL
        OpenSSL::Crypto
        ZLIB::ZLIB
        ${CURL_LIBRARIES}
        ${YKPERS_LIBRARIES}
    )
endif()

# Install rules
install(TARGETS ${PROGNAME}
        BUNDLE DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(UNIX AND NOT APPLE)
    install(FILES src/core/keepassxc.xdg DESTINATION ${CMAKE_INSTALL_XDG_CONFIGDIR}/autostart RENAME org.keepassxc.KeePassXC.desktop)
    install(FILES src/core/keepassxc.xdg DESTINATION ${CMAKE_INSTALL_APPDIR} RENAME org.keepassxc.KeePassXC.desktop)
    install(FILES src/core/keepassxc.xdg DESTINATION ${CMAKE_INSTALL_APPDIR} RENAME org.keepassxc.KeePassXC.desktop)
    install(FILES src/core/keepassxc.png DESTINATION ${CMAKE_INSTALL_ICONDIR}/hicolor/256x256/apps RENAME keepassxc.png)
    install(FILES src/core/keepassxc.svg DESTINATION ${CMAKE_INSTALL_ICONDIR}/hicolor/scalable/apps RENAME keepassxc.svg)
    install(FILES src/core/keepassxc.xml DESTINATION ${CMAKE_INSTALL_APPDATADIR} RENAME org.keepassxc.KeePassXC.appdata.xml)
    install(FILES src/core/keepassxc.appdata.xml DESTINATION ${CMAKE_INSTALL_APPDATADIR} RENAME org.keepassxc.KeePassXC.appdata.xml)
    install(FILES src/core/keepassxc.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
    install(FILES src/core/keepassxc.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1 RENAME kpxc.1)
    
    if(WITH_XC_FDOSECRETS)
        install(FILES src/fdosecrets/org.freedesktop.secrets.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/interfaces RENAME org.keepassxc.KeePassXC.xml)
        install(FILES src/fdosecrets/org.keepassxc.KeePassXC.conf DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services)
    endif()
endif()

if(APPLE)
    install(TARGETS ${PROGNAME} BUNDLE DESTINATION .)
    install(FILES src/core/keepassxc.icns DESTINATION ${PROGNAME}.app/Contents/Resources)
    install(FILES src/core/keepassxc.png DESTINATION ${PROGNAME}.app/Contents/Resources)
    install(FILES src/core/keepassxc.svg DESTINATION ${PROGNAME}.app/Contents/Resources)
    install(FILES src/core/Info.plist DESTINATION ${PROGNAME}.app/Contents)
    install(FILES src/core/keepassxc.entitlements DESTINATION ${PROGNAME}.app/Contents)
    install(FILES src/core/keepassxc.xdg DESTINATION ${PROGNAME}.app/Contents/Resources)
    install(FILES src/core/keepassxc.appdata.xml DESTINATION ${CMAKE_INSTALL_APPDATADIR})
    install(FILES src/core/keepassxc.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif()

# Tests
if(WITH_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Documentation
add_subdirectory(docs)

# Include some nice helper cmake modules
include(CopyUsefulFonts)
include(GenerateProductVersion)
include(CompilerFlags)

# Add CPack packaging support
include(CPackConfig)