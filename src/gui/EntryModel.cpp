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
    Q_ASSERT(index.isValid() && index.row() < m_group->entries().size());
    return m_group->entries().at(index.row());
}

void EntryModel::setGroup(Group* group)
{
    beginResetModel();

    if (m_group) {
        disconnect(m_group, 0, this, 0);
    }
    m_group = group;
    connect(group, SIGNAL(entryAboutToAdd(Entry*)), SLOT(entryAboutToAdd(Entry*)));
    connect(group, SIGNAL(entryAdded()), SLOT(entryAdded()));
    connect(group, SIGNAL(entryAboutToRemove(Entry*)), SLOT(entryAboutToRemove(Entry*)));
    connect(group, SIGNAL(entryRemoved()), SLOT(entryRemoved()));
    connect(group, SIGNAL(entryDataChanged(Entry*)), SLOT(entryDataChanged(Entry*)));

    endResetModel();
}

int EntryModel::rowCount(const QModelIndex& parent) const
{
    if (!m_group || parent.isValid()) {
        return 0;
    }
    else {
        return m_group->entries().size();
    }
}

int EntryModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    return 3;
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
            return entry->title();
        case 1:
            return entry->username();
        case 2:
            return entry->url();
        }
    }
    else if ((role == Qt::DecorationRole) && (index.column() == 0)) {
        return entry->iconPixmap();
    }

    return QVariant();
}
QVariant EntryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0:
            return tr("Title");
        case 1:
            return tr("Username");
        case 2:
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
        stream << m_group->database()->uuid() << entryFromIndex(indexes[i])->uuid();
    }

    data->setData(mimeTypes().first(), encoded);
    return data;
}

void EntryModel::entryAboutToAdd(Entry* entry)
{
    Q_UNUSED(entry);

    beginInsertRows(QModelIndex(), m_group->entries().size(), m_group->entries().size());
}

void EntryModel::entryAdded()
{
    endInsertRows();
}

void EntryModel::entryAboutToRemove(Entry* entry)
{
    beginRemoveRows(QModelIndex(), m_group->entries().indexOf(entry), m_group->entries().indexOf(entry));
}

void EntryModel::entryRemoved()
{
    endRemoveRows();
}

void EntryModel::entryDataChanged(Entry* entry)
{
    int row = m_group->entries().indexOf(entry);
    Q_EMIT dataChanged(index(row, 0), index(row, columnCount()-1));
}
