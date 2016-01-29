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

#include "EntryModel.h"

#include <QFont>
#include <QMimeData>

#include "core/DatabaseIcons.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"

EntryModel::EntryModel(QObject* parent)
    : QAbstractTableModel(parent)
    , m_group(nullptr)
{
}

Entry* EntryModel::entryFromIndex(const QModelIndex& index) const
{
    Q_ASSERT(index.isValid() && index.row() < m_entries.size());
    return m_entries.at(index.row());
}

QModelIndex EntryModel::indexFromEntry(Entry* entry) const
{
    int row = m_entries.indexOf(entry);
    Q_ASSERT(row != -1);
    return index(row, 1);
}

void EntryModel::setGroup(Group* group)
{
    if (!group || group == m_group) {
        return;
    }

    beginResetModel();

    severConnections();

    m_group = group;
    m_allGroups.clear();
    m_entries = group->entries();
    m_orgEntries.clear();

    makeConnections(group);

    endResetModel();
    Q_EMIT switchedToGroupMode();
}

void EntryModel::setEntryList(const QList<Entry*>& entries)
{
    beginResetModel();

    severConnections();

    m_group = nullptr;
    m_allGroups.clear();
    m_entries = entries;
    m_orgEntries = entries;

    QSet<Database*> databases;

    Q_FOREACH (Entry* entry, m_entries) {
        databases.insert(entry->group()->database());
    }

    Q_FOREACH (Database* db, databases) {
        Q_ASSERT(db);
        Q_FOREACH (const Group* group, db->rootGroup()->groupsRecursive(true)) {
            m_allGroups.append(group);
        }

        if (db->metadata()->recycleBin()) {
            m_allGroups.removeOne(db->metadata()->recycleBin());
        }
    }

    Q_FOREACH (const Group* group, m_allGroups) {
        makeConnections(group);
    }

    endResetModel();
    Q_EMIT switchedToEntryListMode();
}

int EntryModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    else {
        return m_entries.size();
    }
}

int EntryModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    return 4;
}

QVariant EntryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    Entry* entry = entryFromIndex(index);

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ParentGroup:
            if (entry->group()) {
                return entry->group()->name();
            }
            break;
        case Title:
            return entry->title();
        case Username:
            return entry->username();
        case Url:
            return entry->url();
        }
    }
    else if (role == Qt::DecorationRole) {
        switch (index.column()) {
        case ParentGroup:
            if (entry->group()) {
                return entry->group()->iconScaledPixmap();
            }
            break;
        case Title:
            if (entry->isExpired()) {
                return databaseIcons()->iconPixmap(DatabaseIcons::ExpiredIconIndex);
            }
            else {
                return entry->iconScaledPixmap();
            }
        }
    }
    else if (role == Qt::FontRole) {
        QFont font;
        if (entry->isExpired()) {
            font.setStrikeOut(true);
        }
        return font;
    }

    return QVariant();
}
QVariant EntryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case ParentGroup:
            return tr("Group");
        case Title:
            return tr("Title");
        case Username:
            return tr("Username");
        case Url:
            return tr("URL");
        }
    }

    return QVariant();
}

Qt::DropActions EntryModel::supportedDropActions() const
{
    return 0;
}

Qt::DropActions EntryModel::supportedDragActions() const
{
    return (Qt::MoveAction | Qt::CopyAction);
}

Qt::ItemFlags EntryModel::flags(const QModelIndex& modelIndex) const
{
    if (!modelIndex.isValid()) {
        return Qt::NoItemFlags;
    }
    else {
        return QAbstractItemModel::flags(modelIndex) | Qt::ItemIsDragEnabled;
    }
}

QStringList EntryModel::mimeTypes() const
{
    QStringList types;
    types << QLatin1String("application/x-keepassx-entry");
    return types;
}

QMimeData* EntryModel::mimeData(const QModelIndexList& indexes) const
{
    if (indexes.isEmpty()) {
        return nullptr;
    }

    QMimeData* data = new QMimeData();
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);

    QSet<Entry*> seenEntries;

    Q_FOREACH (const QModelIndex& index, indexes) {
        if (!index.isValid()) {
            continue;
        }

        Entry* entry = entryFromIndex(index);
        if (!seenEntries.contains(entry)) {
            // make sure we don't add entries multiple times when we get indexes
            // with the same row but different columns
            stream << entry->group()->database()->uuid() << entry->uuid();
            seenEntries.insert(entry);
        }
    }

    if (seenEntries.isEmpty()) {
        delete data;
        return nullptr;
    }
    else {
        data->setData(mimeTypes().at(0), encoded);
        return data;
    }
}

void EntryModel::entryAboutToAdd(Entry* entry)
{
    if (!m_group && !m_orgEntries.contains(entry)) {
        return;
    }

    beginInsertRows(QModelIndex(), m_entries.size(), m_entries.size());
    if (!m_group) {
        m_entries.append(entry);
    }
}

void EntryModel::entryAdded(Entry* entry)
{
    if (!m_group && !m_orgEntries.contains(entry)) {
        return;
    }

    if (m_group) {
        m_entries = m_group->entries();
    }
    endInsertRows();
}

void EntryModel::entryAboutToRemove(Entry* entry)
{
    beginRemoveRows(QModelIndex(), m_entries.indexOf(entry), m_entries.indexOf(entry));
    if (!m_group) {
        m_entries.removeAll(entry);
    }
}

void EntryModel::entryRemoved()
{
    if (m_group) {
        m_entries = m_group->entries();
    }

    endRemoveRows();
}

void EntryModel::entryDataChanged(Entry* entry)
{
    int row = m_entries.indexOf(entry);
    Q_EMIT dataChanged(index(row, 0), index(row, columnCount()-1));
}

void EntryModel::severConnections()
{
    if (m_group) {
        disconnect(m_group, nullptr, this, nullptr);
    }

    Q_FOREACH (const Group* group, m_allGroups) {
        disconnect(group, nullptr, this, nullptr);
    }
}

void EntryModel::makeConnections(const Group* group)
{
    connect(group, SIGNAL(entryAboutToAdd(Entry*)), SLOT(entryAboutToAdd(Entry*)));
    connect(group, SIGNAL(entryAdded(Entry*)), SLOT(entryAdded(Entry*)));
    connect(group, SIGNAL(entryAboutToRemove(Entry*)), SLOT(entryAboutToRemove(Entry*)));
    connect(group, SIGNAL(entryRemoved(Entry*)), SLOT(entryRemoved()));
    connect(group, SIGNAL(entryDataChanged(Entry*)), SLOT(entryDataChanged(Entry*)));
}
