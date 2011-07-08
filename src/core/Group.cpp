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

#include "DatabaseIcons.h"
#include "Metadata.h"

Group::Group()
{
    m_parent = 0;
    m_db = 0;
    m_lastTopVisibleEntry = 0;

    m_iconNumber = 48;
    m_isExpanded = true;
    m_autoTypeEnabled = Inherit;
    m_searchingEnabled = Inherit;
}

Group::~Group()
{
    // TODO notify parent
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

QIcon Group::icon() const
{
    if (m_customIcon.isNull()) {
        return DatabaseIcons::icon(m_iconNumber);
    }
    else {
        return m_db->metadata()->customIcon(m_customIcon);
    }
}

int Group::iconNumber() const
{
    return m_iconNumber;
}

Uuid Group::iconUuid() const
{
    return m_customIcon;
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

Group::TriState Group::autoTypeEnabled() const
{
    return m_autoTypeEnabled;
}

Group::TriState Group::searchingEnabed() const
{
    return m_searchingEnabled;
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

void Group::setAutoTypeEnabled(TriState enable)
{
    m_autoTypeEnabled = enable;
}

void Group::setSearchingEnabled(TriState enable)
{
    m_searchingEnabled = enable;
}

void Group::setLastTopVisibleEntry(Entry* entry)
{
    m_lastTopVisibleEntry = entry;
}

Group* Group::parentGroup()
{
    return m_parent;
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
}

const Database* Group::database() const
{
    return m_db;
}

QList<Group*> Group::children()
{
    return m_children;
}

const QList<Group*>& Group::children() const
{
    return m_children;
}

QList<Entry*> Group::entries()
{
    return m_entries;
}

const QList<Entry*>& Group::entries() const
{
    return m_entries;
}

void Group::addEntry(Entry *entry)
{
    Q_ASSERT(entry);

    Q_EMIT entryAboutToAdd(entry);

    m_entries << entry;
    connect(entry, SIGNAL(dataChanged(Entry*)), SIGNAL(entryDataChanged(Entry*)));

    Q_EMIT entryAdded();
}

void Group::removeEntry(Entry* entry)
{
    Q_EMIT entryAboutToRemove(entry);

    entry->disconnect(this);
    m_entries.removeAll(entry);

    Q_EMIT entryRemoved();
}

void Group::recSetDatabase(Database* db)
{
    disconnect(SIGNAL(dataChanged(Group*)), m_db);
    disconnect(SIGNAL(aboutToRemove(Group*)), m_db);
    disconnect(SIGNAL(removed()), m_db);
    disconnect(SIGNAL(aboutToAdd(Group*,int)), m_db);
    disconnect(SIGNAL(added()), m_db);

    connect(this, SIGNAL(dataChanged(Group*)), db, SIGNAL(groupDataChanged(Group*)));
    connect(this, SIGNAL(aboutToRemove(Group*)), db, SIGNAL(groupAboutToRemove(Group*)));
    connect(this, SIGNAL(removed()), db, SIGNAL(groupRemoved()));
    connect(this, SIGNAL(aboutToAdd(Group*,int)), db, SIGNAL(groupAboutToAdd(Group*,int)));
    connect(this, SIGNAL(added()), db, SIGNAL(groupAdded()));

    m_db = db;

    Q_FOREACH (Group* group, m_children) {
        group->recSetDatabase(db);
    }
}
