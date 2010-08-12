/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

#include "Group.h"

#include "Database.h"

Group::Group() : m_parent(0)
{
}

Uuid Group::uuid() const
{
    return m_uuid;
}

QString Group::name() const
{
    return m_name;
}

QString Group::notes() const
{
    return m_notes;
}

QImage Group::icon() const
{
    if (m_iconNumber == 0)
        return m_db->customIcon(m_customIcon);
    else
        return Database::icon(m_iconNumber);
}

TimeInfo Group::timeInfo() const
{
    return m_timeInfo;
}

bool Group::isExpanded() const
{
    return m_isExpanded;
}

QString Group::defaultAutoTypeSequence() const
{
    return m_defaultAutoTypeSequence;
}

Entry* Group::lastTopVisibleEntry() const
{
    return m_lastTopVisibleEntry;
}

void Group::setUuid(const Uuid& uuid)
{
    m_uuid = uuid;
}

void Group::setName(const QString& name)
{
    m_name = name;
}

void Group::setNotes(const QString& notes)
{
    m_notes = notes;
}

void Group::setIcon(int iconNumber)
{
    m_iconNumber = iconNumber;
    m_customIcon = Uuid();
}

void Group::setIcon(const Uuid& uuid)
{
    m_iconNumber = 0;
    m_customIcon = uuid;
}

void Group::setTimeInfo(const TimeInfo& timeInfo)
{
    m_timeInfo = timeInfo;
}

void Group::setExpanded(bool expanded)
{
    m_isExpanded = expanded;
}

void Group::setDefaultAutoTypeSequence(const QString& sequence)
{
    m_defaultAutoTypeSequence = sequence;
}

void Group::setLastTopVisibleEntry(Entry* entry)
{
    m_lastTopVisibleEntry = entry;
}

void Group::setParent(Group* parent)
{
    if (m_parent) {
        m_parent->m_children.removeAll(this);
    }

    m_db = parent->m_db;
    QObject::setParent(parent);
}

void Group::setParent(Database* db)
{
    if (m_db) {
        m_db->setRootGroup(0);
    }

    m_parent = 0;
    m_db = db;
    QObject::setParent(db);
}

QList<Group*> Group::children() const
{
    return m_children;
}

QList<Entry*> Group::entries() const
{
    return m_entries;
}

void Group::addEntry(Entry *entry)
{
    m_entries << entry;
}

void Group::removeEntry(Entry* entry)
{
    m_entries.removeAll(entry);
}
