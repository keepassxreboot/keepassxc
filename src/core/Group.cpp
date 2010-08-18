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

Group::Group()
{
    m_parent = 0;
    m_db = 0;
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

    Q_EMIT dataChanged(this);
}

void Group::setNotes(const QString& notes)
{
    m_notes = notes;
}

void Group::setIcon(int iconNumber)
{
    Q_ASSERT(iconNumber >= 0);

    m_iconNumber = iconNumber;
    m_customIcon = Uuid();

    Q_EMIT dataChanged(this);
}

void Group::setIcon(const Uuid& uuid)
{
    Q_ASSERT(!uuid.isNull());

    m_iconNumber = 0;
    m_customIcon = uuid;

    Q_EMIT dataChanged(this);
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

const Group* Group::parentGroup() const
{
    return m_parent;
}

void Group::setParent(Group* parent, int index)
{
    Q_ASSERT(parent);
    Q_ASSERT(index >= -1 && index <= parent->children().size());

    if (index == -1) {
        index = parent->children().size();
    }

    Q_EMIT aboutToRemove(this);

    if (m_parent) {
        m_parent->m_children.removeAll(this);
    }
    else if (m_db) {
        // parent was a Database
        m_db->setRootGroup(0);
    }

    Q_EMIT removed();

    m_parent = parent;

    if (m_db != parent->m_db) {
        recSetDatabase(parent->m_db);
    }

    QObject::setParent(parent);

    Q_EMIT aboutToAdd(this, index);

    parent->m_children.insert(index, this);

    Q_EMIT added();
}

void Group::setParent(Database* db)
{
    Q_ASSERT(db);

    Q_EMIT aboutToRemove(this);

    if (m_parent) {
        m_parent->m_children.removeAll(this);
    }
    else if (m_db) {
        m_db->setRootGroup(0);
    }

    Q_EMIT removed();

    m_parent = 0;
    recSetDatabase(db);

    QObject::setParent(db);

    db->setRootGroup(this);
}

const Database* Group::database() const
{
    return m_db;
}

QList<Group*> Group::children()
{
    return m_children;
}

QList<const Group*> Group::children() const
{
    QList<const Group*> constChildren;
    Q_FOREACH (Group* group, m_children) {
        constChildren << group;
    }

    return constChildren;
}

QList<Entry*> Group::entries()
{
    return m_entries;
}

QList<const Entry*> Group::entries() const
{
    QList<const Entry*> constEntries;
    Q_FOREACH (Entry* entry, m_entries) {
        constEntries << entry;
    }

    return constEntries;
}

void Group::addEntry(Entry *entry)
{
    Q_ASSERT(entry != 0);

    m_entries << entry;
}

void Group::removeEntry(Entry* entry)
{
    m_entries.removeAll(entry);
}

void Group::recSetDatabase(Database* db)
{
    disconnect(SIGNAL(dataChanged(const Group*)), m_db);
    disconnect(SIGNAL(aboutToRemove(const Group*)), m_db);
    disconnect(SIGNAL(removed()), m_db);
    disconnect(SIGNAL(aboutToAdd(const Group*,int)), m_db);
    disconnect(SIGNAL(added()), m_db);

    connect(this, SIGNAL(dataChanged(const Group*)), db, SIGNAL(groupDataChanged(const Group*)));
    connect(this, SIGNAL(aboutToRemove(const Group*)), db, SIGNAL(groupAboutToRemove(const Group*)));
    connect(this, SIGNAL(removed()), db, SIGNAL(groupRemoved()));
    connect(this, SIGNAL(aboutToAdd(const Group*,int)), db, SIGNAL(groupAboutToAdd(const Group*,int)));
    connect(this, SIGNAL(added()), db, SIGNAL(groupAdded()));

    m_db = db;

    Q_FOREACH (Group* group, m_children) {
        group->recSetDatabase(db);
    }
}
