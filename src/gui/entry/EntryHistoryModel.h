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

#ifndef KEEPASSX_ENTRYHISTORYMODEL_H
#define KEEPASSX_ENTRYHISTORYMODEL_H

#include <QAbstractTableModel>

class Entry;

class EntryHistoryModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit EntryHistoryModel(QObject* parent = nullptr);

    Entry* entryFromIndex(const QModelIndex& index) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setEntries(const QList<Entry*>& entries, Entry* parentEntry);
    void clear();
    void clearDeletedEntries();
    QList<Entry*> deletedEntries();
    void deleteIndex(QModelIndex index);
    void deleteAll();

private:
    void calculateHistoryModifications();

    QList<Entry*> m_historyEntries;
    QList<Entry*> m_deletedHistoryEntries;
    QStringList m_historyModifications;
    const Entry* m_parentEntry;
};

#endif // KEEPASSX_ENTRYHISTORYMODEL_H
