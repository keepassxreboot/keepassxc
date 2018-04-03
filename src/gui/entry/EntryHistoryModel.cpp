/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "EntryHistoryModel.h"

#include "core/Entry.h"
#include "core/Global.h"

EntryHistoryModel::EntryHistoryModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

Entry* EntryHistoryModel::entryFromIndex(const QModelIndex& index) const
{
    Q_ASSERT(index.isValid() && index.row() < m_historyEntries.size());
    return m_historyEntries.at(index.row());
}

int EntryHistoryModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 4;
}

int EntryHistoryModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        return m_historyEntries.count();
    } else {
        return 0;
    }
}

QVariant EntryHistoryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole || role == Qt::UserRole) {
        Entry* entry = entryFromIndex(index);
        TimeInfo timeInfo = entry->timeInfo();
        QDateTime lastModificationLocalTime = timeInfo.lastModificationTime().toLocalTime();
        switch (index.column()) {
        case 0:
            if (role == Qt::DisplayRole) {
                return lastModificationLocalTime.toString(Qt::SystemLocaleShortDate);
            } else {
                return lastModificationLocalTime;
            }
        case 1:
            return entry->title();
        case 2:
            return entry->username();
        case 3:
            return entry->url();
        }
    }

    return QVariant();
}

QVariant EntryHistoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0:
            return tr("Last modified");
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

void EntryHistoryModel::setEntries(const QList<Entry*>& entries)
{
    beginResetModel();

    m_historyEntries = entries;
    m_deletedHistoryEntries.clear();

    endResetModel();
}

void EntryHistoryModel::clear()
{
    beginResetModel();

    m_historyEntries.clear();
    m_deletedHistoryEntries.clear();

    endResetModel();
}

void EntryHistoryModel::clearDeletedEntries()
{
    m_deletedHistoryEntries.clear();
}

QList<Entry*> EntryHistoryModel::deletedEntries()
{
    return m_deletedHistoryEntries;
}

void EntryHistoryModel::deleteIndex(QModelIndex index)
{
    if (index.isValid()) {
        Entry* entry = entryFromIndex(index);
        beginRemoveRows(QModelIndex(), m_historyEntries.indexOf(entry), m_historyEntries.indexOf(entry));
        m_historyEntries.removeAll(entry);
        m_deletedHistoryEntries << entry;
        endRemoveRows();
    }
}

void EntryHistoryModel::deleteAll()
{
    Q_ASSERT(m_historyEntries.count() > 0);

    beginRemoveRows(QModelIndex(), 0, m_historyEntries.size() - 1);

    for (Entry* entry : asConst(m_historyEntries)) {
        m_deletedHistoryEntries << entry;
    }
    m_historyEntries.clear();
    endRemoveRows();
}
