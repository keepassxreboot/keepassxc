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

#include <QAbstractItemModel>

#include "core/Global.h"

class Database;
class Group;

class GroupModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit GroupModel(Database* db, QObject* parent = Q_NULLPTR);
    void changeDatabase(Database* newDb);
    QModelIndex index(Group* group) const;
    Group* groupFromIndex(const QModelIndex& index) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex& index) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    Qt::DropActions supportedDropActions() const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex& modelIndex) const Q_DECL_OVERRIDE;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column,
                      const QModelIndex& parent) Q_DECL_OVERRIDE;
    QStringList mimeTypes() const Q_DECL_OVERRIDE;
    QMimeData* mimeData(const QModelIndexList& indexes) const Q_DECL_OVERRIDE;

private:
    QModelIndex parent(Group* group) const;

private Q_SLOTS:
    void groupDataChanged(Group* group);
    void groupAboutToRemove(Group* group);
    void groupRemoved();
    void groupAboutToAdd(Group* group, int index);
    void groupAdded();
    void groupAboutToMove(Group* group, Group* toGroup, int pos);
    void groupMoved();

private:
    Database* m_db;
};

#endif // KEEPASSX_GROUPMODEL_H
