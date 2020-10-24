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
endif()
message(STATUS "Using asciidoctor: ${ASCIIDOCTOR_EXE}")

set(DOC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(REV -a revnumber=${KEEPASSXC_VERSION})

# Build html documentation on all platforms
file(GLOB html_depends ${DOC_DIR}/topics/* ${DOC_DIR}/styles/* ${DOC_DIR}/images/*)
add_custom_command(OUTPUT KeePassXC_GettingStarted.html
    COMMAND ${ASCIIDOCTOR_EXE} -D ${OUT_DIR} -o KeePassXC_GettingStarted.html ${REV} ${DOC_DIR}/GettingStarted.adoc
    DEPENDS ${html_depends} ${DOC_DIR}/GettingStarted.adoc
    VERBATIM)
add_custom_command(OUTPUT KeePassXC_UserGuide.html
    COMMAND ${ASCIIDOCTOR_EXE} -D ${OUT_DIR} -o KeePassXC_UserGuide.html ${REV} ${DOC_DIR}/UserGuide.adoc
    DEPENDS ${html_depends} ${DOC_DIR}/UserGuide.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM)
file(GLOB styles_depends ${DOC_DIR}/styles/*)
add_custom_command(OUTPUT KeePassXC_KeyboardShortcuts.html
    COMMAND ${ASCIIDOCTOR_EXE} -D ${OUT_DIR} -o KeePassXC_KeyboardShortcuts.html ${REV} ${DOC_DIR}/topics/KeyboardShortcuts.adoc
    DEPENDS ${DOC_DIR}/topics/KeyboardShortcuts.adoc ${styles_depends}
    VERBATIM)

add_custom_target(docs ALL DEPENDS KeePassXC_GettingStarted.html KeePassXC_UserGuide.html KeePassXC_KeyboardShortcuts.html)

install(FILES
    ${OUT_DIR}/KeePassXC_GettingStarted.html
    ${OUT_DIR}/KeePassXC_UserGuide.html
    ${OUT_DIR}/KeePassXC_KeyboardShortcuts.html
    DESTINATION ${DATA_INSTALL_DIR}/docs)

# Build Man Pages on Linux and macOS
if(UNIX)
    add_custom_command(OUTPUT keepassxc.1
        COMMAND ${ASCIIDOCTOR_EXE} -D ${OUT_DIR} -b manpage ${REV} ${DOC_DIR}/man/keepassxc.1.adoc
        DEPENDS ${DOC_DIR}/man/keepassxc.1.adoc
        VERBATIM)
    add_custom_command(OUTPUT keepassxc-cli.1
        COMMAND ${ASCIIDOCTOR_EXE} -D ${OUT_DIR} -b manpage ${REV} ${DOC_DIR}/man/keepassxc-cli.1.adoc
        DEPENDS ${DOC_DIR}/man/keepassxc-cli.1.adoc
        VERBATIM)
    add_custom_target(manpages ALL DEPENDS keepassxc.1 keepassxc-cli.1)

    install(FILES 
        ${OUT_DIR}/keepassxc.1
        ${OUT_DIR}/keepassxc-cli.1
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1/)
endif()
