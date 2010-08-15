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

class Group;

class GroupModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit GroupModel(const Group* rootGroup, QObject* parent = 0);
    void setRootGroup(const Group* group);

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& index) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    QModelIndex createIndex(int row, int column, const Group* group) const;
    const Group* groupFromIndex(const QModelIndex& index) const;

    const Group* m_root;
};

#endif // KEEPASSX_GROUPMODEL_H
