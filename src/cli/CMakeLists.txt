#  Copyright (C) 2025 KeePassXC Team
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

set(cli_SOURCES
        Add.cpp
        AddGroup.cpp
        Analyze.cpp
        AttachmentExport.cpp
        AttachmentImport.cpp
        AttachmentRemove.cpp
        Clip.cpp
        Close.cpp
        Command.cpp
        DatabaseCommand.cpp
        DatabaseCreate.cpp
        DatabaseEdit.cpp
        DatabaseInfo.cpp
        Diceware.cpp
        Edit.cpp
        Estimate.cpp
        Exit.cpp
        Export.cpp
        Generate.cpp
        Help.cpp
        Import.cpp
        List.cpp
        Merge.cpp
        Move.cpp
        Open.cpp
        Remove.cpp
        RemoveGroup.cpp
        Search.cpp
        Show.cpp)

# text_stream library
add_library(text_stream STATIC TextStream.cpp)
target_link_libraries(text_stream PUBLIC Qt6::Core)

# line_reader library
add_library(line_reader STATIC LineReader.cpp)
target_link_libraries(line_reader PUBLIC Qt6::Core)
find_package(Readline MODULE)
if(Readline_FOUND)
    target_compile_definitions(line_reader PRIVATE "USE_READLINE")
    target_link_libraries(line_reader PRIVATE Readline::readline)
else()
    target_link_libraries(line_reader PRIVATE text_stream)
endif()

# cli library
add_library(cli STATIC ${cli_SOURCES})
target_link_libraries(cli
        PRIVATE ${ZXCVBN_LIBRARIES} text_stream
        PUBLIC Qt6::Core)

add_executable(keepassxc-cli keepassxc-cli.cpp)
target_link_libraries(keepassxc-cli
        PRIVATE ${GPGERROR_LIBRARIES} cli keepassxc_core line_reader)

install(TARGETS keepassxc-cli
        BUNDLE DESTINATION . COMPONENT Runtime
        RUNTIME DESTINATION ${CLI_INSTALL_DIR} COMPONENT Runtime)

if(WIN32)
    target_sources(keepassxc-cli
        PRIVATE keepassxc-cli.exe.manifest)
endif()

if(APPLE AND WITH_APP_BUNDLE)
    set(CLI_APP_DIR "${CMAKE_BINARY_DIR}/src/${CLI_INSTALL_DIR}")

    add_custom_command(TARGET keepassxc-cli
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/keepassxc-cli ${CLI_APP_DIR}/keepassxc-cli
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src
            COMMENT "Copying keepassxc-cli into app bundle")
    set_property(GLOBAL APPEND PROPERTY
        _MACDEPLOYQT_EXTRA_BINARIES "${CLI_INSTALL_DIR}/keepassxc-cli")
endif()
