#  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR})

set(TEST_LIBRARIES keepassx_core)
set(TEST_GUI_SUPPORT_SOURCES
        main.cpp
        ../../tests/util/TemporaryFile.cpp
        FixtureBase.cpp
        FixtureWithDb.cpp)
add_library(TEST_GUI_SUPPORT STATIC ${TEST_GUI_SUPPORT_SOURCES})
target_link_libraries(TEST_GUI_SUPPORT Qt5::Core Qt5::Widgets Qt5::Test Catch2::Catch2)

# Add new unit tests here.
# Naming convention: word-cased separated by a dash. Avoid naming with "test" as obvious.
add_unit_test(NAME pass-phrase-generator-gui
        SOURCES TestPassphraseGeneratorGui.cpp
        LIBS TEST_GUI_SUPPORT ${TEST_LIBRARIES})
