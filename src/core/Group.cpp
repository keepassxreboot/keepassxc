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

#include "core/DatabaseIcons.h"
#include "core/Metadata.h"

Group::Group()
{
    m_parent = 0;
    m_db = 0;
    m_lastTopVisibleEntry = 0;

    m_iconNumber = 48;
    m_isExpanded = true;
    m_autoTypeEnabled = Inherit;
    m_searchingEnabled = Inherit;

    m_updateTimeinfo = true;
}

Group::~Group()
{
    cleanupParent();
}

template <class T> bool Group::set(T& property, const T& value) {
    if (property != value) {
        property = value;
        if (m_updateTimeinfo) {
            m_timeInfo.setLastModificationTime(QDateTime::currentDateTime());
            m_timeInfo.setLastAccessTime(QDateTime::currentDateTime());
        }
        Q_EMIT modified();
        return true;
    }
    else {
        return false;
    }
}

void Group::setUpdateTimeinfo(bool value) {
    m_updateTimeinfo = value;
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
    if (m_customIcon.isNull()) {
        return databaseIcons()->icon(m_iconNumber);
    }
    else {
        // TODO check if m_db is 0
        return m_db->metadata()->customIcon(m_customIcon);
    }
}

QPixmap Group::iconPixmap() const
{
    if (m_customIcon.isNull()) {
        return databaseIcons()->iconPixmap(m_iconNumber);
    }
    else {
        QPixmap pixmap;
        if (!QPixmapCache::find(m_pixmapCacheKey, &pixmap)) {
            // TODO check if m_db is 0
            pixmap = QPixmap::fromImage(m_db->metadata()->customIcon(m_customIcon));
            *const_cast<QPixmapCache::Key*>(&m_pixmapCacheKey) = QPixmapCache::insert(pixmap);
        }

        return pixmap;
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
    set(m_uuid, uuid);
}

void Group::setName(const QString& name)
{
    if (set(m_name, name)) {
        Q_EMIT dataChanged(this);
    }
}

void Group::setNotes(const QString& notes)
{
    set(m_notes, notes);
}

void Group::setIcon(int iconNumber)
{
    Q_ASSERT(iconNumber >= 0);

    if (m_iconNumber != iconNumber || !m_customIcon.isNull()) {
        m_iconNumber = iconNumber;
        m_customIcon = Uuid();

        m_pixmapCacheKey = QPixmapCache::Key();

        Q_EMIT modified();
        Q_EMIT dataChanged(this);
    }
}

void Group::setIcon(const Uuid& uuid)
{
    Q_ASSERT(!uuid.isNull());

    if (set(m_customIcon, uuid)) {
        m_iconNumber = 0;

        m_pixmapCacheKey = QPixmapCache::Key();

        Q_EMIT dataChanged(this);
    }
}

void Group::setTimeInfo(const TimeInfo& timeInfo)
{
    m_timeInfo = timeInfo;
}

void Group::setExpanded(bool expanded)
{
    set(m_isExpanded, expanded);
}

void Group::setDefaultAutoTypeSequence(const QString& sequence)
{
    set(m_defaultAutoTypeSequence, sequence);
}

void Group::setAutoTypeEnabled(TriState enable)
{
    set(m_autoTypeEnabled, enable);
}

void Group::setSearchingEnabled(TriState enable)
{
    set(m_searchingEnabled, enable);
}

void Group::setLastTopVisibleEntry(Entry* entry)
{
    set(m_lastTopVisibleEntry, entry);
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
    // setting a new parent for root groups is not allowed
    Q_ASSERT(!m_db || (m_db->rootGroup() != this));

    if (index == -1) {
        index = parent->children().size();
    }


    cleanupParent();

    m_parent = parent;

    if (m_db != parent->m_db) {
        recSetDatabase(parent->m_db);
    }

    QObject::setParent(parent);

    Q_EMIT aboutToAdd(this, index);

    parent->m_children.insert(index, this);

    Q_EMIT modified();
    Q_EMIT added();
}

void Group::setParent(Database* db)
{
    Q_ASSERT(db);

    cleanupParent();

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
    connect(entry, SIGNAL(modified()), this, SIGNAL(modified()));

    Q_EMIT modified();
    Q_EMIT entryAdded();
}

void Group::removeEntry(Entry* entry)
{
    Q_EMIT entryAboutToRemove(entry);

    entry->disconnect(this);
    m_entries.removeAll(entry);
    Q_EMIT modified();
    Q_EMIT entryRemoved();
}

void Group::recSetDatabase(Database* db)
{
    disconnect(SIGNAL(dataChanged(Group*)), m_db);
    disconnect(SIGNAL(aboutToRemove(Group*)), m_db);
    disconnect(SIGNAL(removed()), m_db);
    disconnect(SIGNAL(aboutToAdd(Group*,int)), m_db);
    disconnect(SIGNAL(added()), m_db);
    disconnect(SIGNAL(modified()), m_db);

    connect(this, SIGNAL(dataChanged(Group*)), db, SIGNAL(groupDataChanged(Group*)));
    connect(this, SIGNAL(aboutToRemove(Group*)), db, SIGNAL(groupAboutToRemove(Group*)));
    connect(this, SIGNAL(removed()), db, SIGNAL(groupRemoved()));
    connect(this, SIGNAL(aboutToAdd(Group*,int)), db, SIGNAL(groupAboutToAdd(Group*,int)));
    connect(this, SIGNAL(added()), db, SIGNAL(groupAdded()));
    connect(this, SIGNAL(modified()), db, SIGNAL(modified()));

    m_db = db;

    Q_FOREACH (Group* group, m_children) {
        group->recSetDatabase(db);
    }
}

void Group::cleanupParent()
{
    if (m_parent) {
        Q_EMIT aboutToRemove(this);
        m_parent->m_children.removeAll(this);
        Q_EMIT modified();
        Q_EMIT removed();
    }
}
