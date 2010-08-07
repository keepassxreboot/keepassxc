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

set(keepassx_SOURCES
    main.cpp
    core/Database.cpp
    core/DbAttribute.cpp
    core/Entry.cpp
    core/Group.cpp
    core/Metadata.cpp
    core/Parser.cpp
    core/TimeInfo.cpp
    core/Uuid.cpp
)

set(keepassx_HEADERS
    core/Database.h
    core/DbAttribute.h
    core/Entry.h
    core/Group.h
    core/Metadata.h
    core/Parser.h
    core/TimeInfo.h
    core/Uuid.h
)

qt4_wrap_cpp( keepassx_MOC ${keepassx_HEADERS} )

add_executable( ${PROGNAME} WIN32 MACOSX_BUNDLE ${keepassx_SOURCES} ${keepassx_MOC} )
target_link_libraries( ${PROGNAME} ${QT_LIBRARIES} )
