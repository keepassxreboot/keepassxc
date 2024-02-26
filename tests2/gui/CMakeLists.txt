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

set(TEST_LIBRARIES keepassx_core)
set(TEST_GUI_SOURCES
        ../../tests/util/TemporaryFile.cpp
        main.cpp
        FixtureBase.cpp
        FixtureWithDb.cpp
        TestPassphraseGeneratorGui.cpp
        TestPasswordGeneratorGui.cpp
        TestAppSettingsGui.cpp
        TestDbCreatingGui.cpp
        TestDbLockingGui.cpp
        TestDbSavingGui.cpp
        TestDbSettingsGui.cpp
        TestTotpGui.cpp)
add_executable(TestGui ${TEST_GUI_SOURCES})
target_link_libraries(TestGui ${TEST_LIBRARIES} Qt5::Test Catch2::Catch2)

add_test(TEST_GUI TestGui)
set_tests_properties(TEST_GUI PROPERTIES ENVIRONMENT "LANG=en_US.UTF-8")
