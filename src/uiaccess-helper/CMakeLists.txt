#  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

if(KPXC_FEATURE_UIACCESS_HELPER)
    # Plain C, no Qt: the binary carries uiAccess. WIN32_EXECUTABLE: no console
    # window. The .rc embeds the manifest, the only form uiAccess is honoured
    # from; MSVC would add a default one, MinGW does not.
    add_executable(keepassxc-uiaccess-helper
            keepassxc-uiaccess-helper.c
            keepassxc-uiaccess-helper.rc)
    target_link_libraries(keepassxc-uiaccess-helper user32 shell32 advapi32)
    set_target_properties(keepassxc-uiaccess-helper PROPERTIES WIN32_EXECUTABLE ON)
    if(MSVC)
        target_link_options(keepassxc-uiaccess-helper PRIVATE "/MANIFEST:NO")
    elseif(MINGW)
        # wWinMain entry point.
        target_link_options(keepassxc-uiaccess-helper PRIVATE "-municode")
    endif()

    # No BUNDLE destination: the target exists only on Windows.
    install(TARGETS keepassxc-uiaccess-helper
            RUNTIME DESTINATION ${BIN_INSTALL_DIR} COMPONENT Runtime)
endif()
