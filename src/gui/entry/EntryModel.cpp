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

#include <QtCore/QMimeData>
#include <QtGui/QFont>

#include "core/DatabaseIcons.h"
#include "core/Entry.h"
#include "core/Group.h"

EntryModel::EntryModel(QObject* parent)
    : QAbstractTableModel(parent)
    , m_group(0)
{
    setSupportedDragActions(Qt::MoveAction);
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
    return index(row, 0);
}

void EntryModel::setGroup(Group* group)
{
    if (!group || group == m_group) {
        return;
    }

    beginResetModel();

    severConnections();

    m_group = group;
    m_entries = group->entries();

    makeConnections(group);

    endResetModel();
    Q_EMIT switchedToView();
}

void EntryModel::setEntries(const QList<Entry*>& entries)
{
    beginResetModel();

    severConnections();

    m_group = 0;
    m_allGroups.clear();
    m_entries = entries;

    if (entries.count() > 0) {
        m_allGroups = entries.at(0)->group()->database()->rootGroup()->groupsRecursive(true);
    }

    Q_FOREACH (const Group* group, m_allGroups) {
        makeConnections(group);
    }

    endResetModel();
    Q_EMIT switchedToSearch();
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
        case 0:
            if (entry->group()) {
                return entry->group()->name();
            }
            break;
        case 1:
            return entry->title();
        case 2:
            return entry->username();
        case 3:
            return entry->url();
        }
    }
    else if (role == Qt::DecorationRole) {
        switch (index.column()) {
        case 0:
            if (entry->group()) {
                return entry->group()->iconPixmap();
            }
            break;
        case 1:
            if (entry->isExpired()) {
                return databaseIcons()->iconPixmap(databaseIcons()->expiredIconIndex());
            }
            else {
                return entry->iconPixmap();
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
        case 0:
            return tr("Group");
        case 1:
            return tr("Title");
        case 2:
            return tr("Username");
        case 3:
            return tr("URL");
        }
    }

    return QVariant();
}

Qt::DropActions EntryModel::supportedDropActions() const
{
    return 0;
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
        return 0;
    }

    QMimeData* data = new QMimeData();
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);

    for (int i = 0; i < indexes.size(); i++) {
        if (!indexes[i].isValid()) {
            continue;
        }
        Entry* entry = entryFromIndex(indexes[i]);
        stream << entry->group()->database()->uuid() << entry->uuid();
    }

    data->setData(mimeTypes().first(), encoded);
    return data;
}

void EntryModel::entryAboutToAdd(Entry* entry)
{
    Q_UNUSED(entry);

    beginInsertRows(QModelIndex(), m_entries.size(), m_entries.size());
    if (!m_group) {
        m_entries.append(entry);
    }
}

void EntryModel::entryAdded()
{
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
        disconnect(m_group, 0, this, 0);
    }

    Q_FOREACH (const Group* group, m_allGroups) {
        disconnect(group, 0, this, 0);
    }
}

void EntryModel::makeConnections(const Group* group)
{
    connect(group, SIGNAL(entryAboutToAdd(Entry*)), SLOT(entryAboutToAdd(Entry*)));
    connect(group, SIGNAL(entryAdded()), SLOT(entryAdded()));
    connect(group, SIGNAL(entryAboutToRemove(Entry*)), SLOT(entryAboutToRemove(Entry*)));
    connect(group, SIGNAL(entryRemoved()), SLOT(entryRemoved()));
    connect(group, SIGNAL(entryDataChanged(Entry*)), SLOT(entryDataChanged(Entry*)));
}
