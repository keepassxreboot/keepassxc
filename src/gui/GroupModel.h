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

#ifndef KEEPASSX_GROUPMODEL_H
#define KEEPASSX_GROUPMODEL_H

#include <QtCore/QAbstractItemModel>

class Database;
class Group;

class GroupModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit GroupModel(const Database* db, QObject* parent = 0);
    QModelIndex index(const Group* group) const;
    const Group* groupFromIndex(const QModelIndex& index) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& index) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

private:
    QModelIndex createIndex(int row, int column, const Group* group) const;
    QModelIndex parent(const Group* group) const;

private Q_SLOTS:
    void groupDataChanged(const Group* group);
    void groupAboutToRemove(const Group* group);
    void groupRemoved();
    void groupAboutToAdd(const Group* group, int index);
    void groupAdded();

private:
    const Group* m_root;
};

#endif // KEEPASSX_GROUPMODEL_H
