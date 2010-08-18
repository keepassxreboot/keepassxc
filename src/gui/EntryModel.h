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

#ifndef KEEPASSX_ENTRYMODEL_H
#define KEEPASSX_ENTRYMODEL_H

#include <QtCore/QAbstractTableModel>

class Entry;
class Group;

class EntryModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit EntryModel(QObject* parent = 0);
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

public Q_SLOTS:
    void setGroup(const Group* group);

private Q_SLOTS:
    void entryAboutToAdd(const Entry* entry);
    void entryAdded();
    void entryAboutToRemove(const Entry* entry);
    void entryRemoved();
    void entryDataChanged(const Entry* entry);

private:
    const Group* m_group;
};

#endif // KEEPASSX_ENTRYMODEL_H
