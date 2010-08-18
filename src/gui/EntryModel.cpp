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

#include "core/Entry.h"
#include "core/Group.h"

EntryModel::EntryModel(QObject* parent)
    : QAbstractTableModel(parent)
    , m_group(0)
{
}

void EntryModel::setGroup(const Group* group)
{
    beginResetModel();

    if (m_group) {
        disconnect(m_group, 0, this, 0);
    }
    m_group = group;
    connect(group, SIGNAL(entryAboutToAdd(const Entry*)), SLOT(entryAboutToAdd(const Entry*)));
    connect(group, SIGNAL(entryAdded()), SLOT(entryAdded()));
    connect(group, SIGNAL(entryAboutToRemove(const Entry*)), SLOT(entryAboutToRemove(const Entry*)));
    connect(group, SIGNAL(entryRemoved()), SLOT(entryRemoved()));
    connect(group, SIGNAL(entryDataChanged(const Entry*)), SLOT(entryDataChanged(const Entry*)));

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

    return 1;
}

QVariant EntryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    const Entry* entry = m_group->entries().at(index.row());

    // TODO implement other columns
    if (role == Qt::DisplayRole) {
        return entry->title();
    }
    else if (role == Qt::DecorationRole) {
        return entry->icon();
    }
    else {
        return QVariant();
    }
}
QVariant EntryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);

    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return tr("Title");
    }

    return QVariant();
}

void EntryModel::entryAboutToAdd(const Entry* entry)
{
    Q_UNUSED(entry);

    beginInsertRows(QModelIndex(), m_group->entries().size(), m_group->entries().size());
}

void EntryModel::entryAdded()
{
    endInsertRows();
}

void EntryModel::entryAboutToRemove(const Entry* entry)
{
    beginRemoveRows(QModelIndex(), m_group->entries().indexOf(entry), m_group->entries().indexOf(entry));
}

void EntryModel::entryRemoved()
{
    endRemoveRows();
}

void EntryModel::entryDataChanged(const Entry* entry)
{
    int row = m_group->entries().indexOf(entry);
    qDebug("%d", index(row, 0).row());
    Q_EMIT dataChanged(index(row, 0), index(row, columnCount()-1));
}
