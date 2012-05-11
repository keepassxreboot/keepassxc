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

#include "Group.h"

#include "core/DatabaseIcons.h"
#include "core/Metadata.h"
#include "core/Tools.h"

Group::Group()
{
    m_iconNumber = 48;
    m_isExpanded = true;
    m_autoTypeEnabled = Inherit;
    m_searchingEnabled = Inherit;

    m_updateTimeinfo = true;
}

Group::~Group()
{
    cleanupParent();
    this->blockSignals(true);
    if (m_db && m_parent) {
        QList<Entry*> entries = m_entries;
        Q_FOREACH (Entry* entry, entries) {
            entry->blockSignals(true);
            delete entry;
        }

        QList<Group*> children = m_children;
        Q_FOREACH (Group* group, children) {
            group->blockSignals(true);
            delete group;
        }

        DeletedObject delGroup;
        delGroup.deletionTime = Tools::currentDateTimeUtc();
        delGroup.uuid = m_uuid;
        m_db->addDeletedObject(delGroup);
    }
}

template <class P, class V> bool Group::set(P& property, const V& value) {
    if (property != value) {
        property = value;
        updateTimeinfo();
        Q_EMIT modified();
        return true;
    }
    else {
        return false;
    }
}

void Group::updateTimeinfo()
{
    if (m_updateTimeinfo) {
        m_timeInfo.setLastModificationTime(Tools::currentDateTimeUtc());
        m_timeInfo.setLastAccessTime(Tools::currentDateTimeUtc());
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
        // TODO: check if m_db is 0
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
            // TODO: check if m_db is 0
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

        updateTimeinfo();
        Q_EMIT modified();
        Q_EMIT dataChanged(this);
    }
}

void Group::setIcon(const Uuid& uuid)
{
    Q_ASSERT(!uuid.isNull());

    if (m_customIcon != uuid) {
        m_customIcon = uuid;
        m_iconNumber = 0;

        m_pixmapCacheKey = QPixmapCache::Key();

        updateTimeinfo();
        Q_EMIT modified();
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

    bool moveWithinDatabase = (m_db && m_db == parent->m_db);

    if (index == -1) {
        index = parent->children().size();

        if (parentGroup() == parent) {
            index--;
        }
    }

    if (m_parent == parent && parent->children().indexOf(this) == index) {
        return;
    }

    if (!moveWithinDatabase) {
        cleanupParent();
        m_parent = parent;
        if (m_db) {
            recCreateDelObjects();

            // copy custom icon to the new database
            if (!iconUuid().isNull() && parent->m_db
                    && m_db->metadata()->containsCustomIcon(iconUuid())
                    && !parent->m_db->metadata()->containsCustomIcon(iconUuid())) {
                parent->m_db->metadata()->addCustomIcon(iconUuid(), icon());
            }
        }
        if (m_db != parent->m_db) {
            recSetDatabase(parent->m_db);
        }
        QObject::setParent(parent);
        Q_EMIT aboutToAdd(this, index);
        Q_ASSERT(index <= parent->m_children.size());
        parent->m_children.insert(index, this);
    }
    else {
        Q_EMIT aboutToMove(this, parent, index);
        m_parent->m_children.removeAll(this);
        m_parent = parent;
        QObject::setParent(parent);
        Q_ASSERT(index <= parent->m_children.size());
        parent->m_children.insert(index, this);
    }

    if (m_updateTimeinfo) {
        m_timeInfo.setLocationChanged(Tools::currentDateTimeUtc());
    }

    Q_EMIT modified();

    if (!moveWithinDatabase) {
        Q_EMIT added();
    }
    else {
        Q_EMIT moved();
    }
}

void Group::setParent(Database* db)
{
    Q_ASSERT(db);
    Q_ASSERT(db->rootGroup() == this);

    cleanupParent();

    m_parent = 0;
    recSetDatabase(db);

    QObject::setParent(db);
}

Database* Group::database()
{
    return m_db;
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

QList<Entry*> Group::entriesRecursive(bool includeHistoryItems)
{
    QList<Entry*> entryList;

    entryList.append(m_entries);

    if (includeHistoryItems) {
        Q_FOREACH (Entry* entry, m_entries) {
            entryList.append(entry->historyItems());
        }
    }

    Q_FOREACH (Group* group, m_children) {
        entryList.append(group->entriesRecursive(includeHistoryItems));
    }

    return entryList;
}

void Group::addEntry(Entry* entry)
{
    Q_ASSERT(entry);
    Q_ASSERT(!m_entries.contains(entry));

    Q_EMIT entryAboutToAdd(entry);

    m_entries << entry;
    connect(entry, SIGNAL(dataChanged(Entry*)), SIGNAL(entryDataChanged(Entry*)));
    if (m_db) {
        connect(entry, SIGNAL(modified()), m_db, SIGNAL(modified()));
    }

    Q_EMIT modified();
    Q_EMIT entryAdded();
}

void Group::removeEntry(Entry* entry)
{
    Q_ASSERT(m_entries.contains(entry));

    Q_EMIT entryAboutToRemove(entry);

    entry->disconnect(this);
    if (m_db) {
        entry->disconnect(m_db);
    }
    m_entries.removeAll(entry);
    Q_EMIT modified();
    Q_EMIT entryRemoved();
}

void Group::recSetDatabase(Database* db)
{
    if (m_db) {
        disconnect(SIGNAL(dataChanged(Group*)), m_db);
        disconnect(SIGNAL(aboutToRemove(Group*)), m_db);
        disconnect(SIGNAL(removed()), m_db);
        disconnect(SIGNAL(aboutToAdd(Group*,int)), m_db);
        disconnect(SIGNAL(added()), m_db);
        disconnect(SIGNAL(aboutToMove(Group*,Group*,int)), m_db);
        disconnect(SIGNAL(moved()), m_db);
        disconnect(SIGNAL(modified()), m_db);
    }

    Q_FOREACH (Entry* entry, m_entries) {
        if (m_db) {
            entry->disconnect(m_db);
        }
        if (db) {
            connect(entry, SIGNAL(modified()), db, SIGNAL(modified()));
        }
    }

    if (db) {
        connect(this, SIGNAL(dataChanged(Group*)), db, SIGNAL(groupDataChanged(Group*)));
        connect(this, SIGNAL(aboutToRemove(Group*)), db, SIGNAL(groupAboutToRemove(Group*)));
        connect(this, SIGNAL(removed()), db, SIGNAL(groupRemoved()));
        connect(this, SIGNAL(aboutToAdd(Group*,int)), db, SIGNAL(groupAboutToAdd(Group*,int)));
        connect(this, SIGNAL(added()), db, SIGNAL(groupAdded()));
        connect(this, SIGNAL(aboutToMove(Group*,Group*,int)), db, SIGNAL(groupAboutToMove(Group*,Group*,int)));
        connect(this, SIGNAL(moved()), db, SIGNAL(groupMoved()));
        connect(this, SIGNAL(modified()), db, SIGNAL(modified()));
    }

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

void Group::recCreateDelObjects()
{
    if (m_db) {
        Q_FOREACH (Entry* entry, m_entries) {
            m_db->addDeletedObject(entry->uuid());
        }

        Q_FOREACH (Group* group, m_children) {
            group->recCreateDelObjects();
        }
        m_db->addDeletedObject(m_uuid);
    }
}
