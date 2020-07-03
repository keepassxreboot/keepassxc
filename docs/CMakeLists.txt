#  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

find_program(ASCIIDOCTOR_EXE asciidoctor)
if(NOT ASCIIDOCTOR_EXE)
    message(FATAL_ERROR "asciidoctor is required to build documentation")
else()
    message(STATUS "Using asciidoctor: ${ASCIIDOCTOR_EXE}")
endif()

# Build html documentation on all platforms
add_custom_command(OUTPUT KeePassXC_GettingStarted.html
    COMMAND ${ASCIIDOCTOR_EXE} -D ${CMAKE_CURRENT_BINARY_DIR} -o KeePassXC_GettingStarted.html GettingStarted.adoc
    DEPENDS topics/* styles/* images/* GettingStarted.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM)
add_custom_command(OUTPUT KeePassXC_UserGuide.html
    COMMAND ${ASCIIDOCTOR_EXE} -D ${CMAKE_CURRENT_BINARY_DIR} -o KeePassXC_UserGuide.html UserGuide.adoc
    DEPENDS topics/* styles/* images/* UserGuide.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM)
add_custom_command(OUTPUT KeePassXC_KeyboardShortcuts.html
    COMMAND ${ASCIIDOCTOR_EXE} -D ${CMAKE_CURRENT_BINARY_DIR} -o KeePassXC_KeyboardShortcuts.html topics/KeyboardShortcuts.adoc
    DEPENDS topics/KeyboardShortcuts.adoc styles/*
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM)

add_custom_target(docs ALL DEPENDS KeePassXC_GettingStarted.html KeePassXC_UserGuide.html KeePassXC_KeyboardShortcuts.html)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/KeePassXC_GettingStarted.html
    ${CMAKE_CURRENT_BINARY_DIR}/KeePassXC_UserGuide.html
    ${CMAKE_CURRENT_BINARY_DIR}/KeePassXC_KeyboardShortcuts.html
    DESTINATION ${DATA_INSTALL_DIR}/docs)
