#  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

if(WITH_XC_BROWSER)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src/safariwebextension)

    set(browser_SOURCES
            BrowserAccessControlDialog.cpp
            BrowserAction.cpp
            BrowserEntryConfig.cpp
            BrowserEntrySaveDialog.cpp
            BrowserHost.cpp
            BrowserMessageBuilder.cpp
            BrowserSettingsPage.cpp
            BrowserSettingsWidget.cpp
            BrowserService.cpp
            BrowserSettings.cpp
            BrowserShared.cpp
            CustomTableWidget.cpp
            NativeMessageInstaller.cpp)

    if(APPLE)
        list(APPEND browser_SOURCES
            BrowserSharedMac.mm)

        if(WITH_XC_SAFARI_WEB_EXTENSION)
            list(APPEND browser_SOURCES
                ../safariwebextension/SafariWebExtensionHelper.mm
                ../safariwebextension/SafariWebExtensionCheckbox.mm)
        endif()
    endif()

    if(WITH_XC_BROWSER_PASSKEYS)
        list(APPEND browser_SOURCES
                BrowserCbor.cpp
                BrowserPasskeys.cpp
                BrowserPasskeysClient.cpp
                BrowserPasskeysConfirmationDialog.cpp
                PasskeyUtils.cpp)
    endif()

    add_library(browser STATIC ${browser_SOURCES})

    if(APPLE)
        target_compile_definitions(browser PRIVATE
            APP_GROUP_IDENTIFIER="${APPLE_TEAM_ID}.${APPLE_APP_IDENTIFIER}" APPLE_APP_IDENTIFIER="${APPLE_APP_IDENTIFIER}")
    endif()

    if(WITH_XC_SAFARI_WEB_EXTENSION)
        target_compile_definitions(browser PRIVATE WITH_XC_SAFARI_WEB_EXTENSION)
    endif()

    target_link_libraries(browser Qt5::Core Qt5::Concurrent Qt5::Widgets Qt5::Network ${BOTAN_LIBRARIES})
endif()
