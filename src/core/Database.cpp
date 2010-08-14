/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Database.h"

#include <QtCore/QFile>
#include <QtCore/QXmlStreamReader>

#include "Metadata.h"
#include "Parser.h"

Database::Database()
{
    m_metadata = new Metadata(this);
}

Group* Database::rootGroup()
{
    return m_rootGroup;
}

void Database::setRootGroup(Group* group)
{
    Q_ASSERT(group == 0 || group->parent() == this);
    m_rootGroup = group;
}

Metadata* Database::metadata()
{
    return m_metadata;
}

QImage Database::icon(int number)
{
    // TODO implement
    return QImage();
}

QImage Database::customIcon(const Uuid& uuid) const
{
    return m_customIcons[uuid];
}

Entry* Database::resolveEntry(const Uuid& uuid)
{
    return recFindEntry(uuid, m_rootGroup);
}

Entry* Database::recFindEntry(const Uuid& uuid, Group* group)
{
    Q_FOREACH (Entry* entry, group->entries()) {
        if (entry->uuid() == uuid)
            return entry;
    }

    Q_FOREACH (Group* child, group->children()) {
        Entry* result = recFindEntry(uuid, child);
        if (result)
            return result;
    }

    return 0;
}

Group* Database::resolveGroup(const Uuid& uuid)
{
    return recFindGroup(uuid, m_rootGroup);
}

Group* Database::recFindGroup(const Uuid& uuid, Group* group)
{
    if (group->uuid() == uuid)
        return group;

    Q_FOREACH (Group* child, group->children()) {
       Group* result = recFindGroup(uuid, child);
       if (result)
           return result;
    }

    return 0;
}
