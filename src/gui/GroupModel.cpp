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

#include "GroupModel.h"

#include "core/Database.h"
#include "core/Group.h"

GroupModel::GroupModel(Database* db, QObject* parent)
    : QAbstractItemModel(parent)
{
    m_root = db->rootGroup();
    connect(db, SIGNAL(groupDataChanged(Group*)), SLOT(groupDataChanged(Group*)));
    connect(db, SIGNAL(groupAboutToAdd(Group*,int)), SLOT(groupAboutToAdd(Group*,int)));
    connect(db, SIGNAL(groupAdded()), SLOT(groupAdded()));
    connect(db, SIGNAL(groupAboutToRemove(Group*)), SLOT(groupAboutToRemove(Group*)));
    connect(db, SIGNAL(groupRemoved()), SLOT(groupRemoved()));
}

int GroupModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        // we have exactly 1 root item
        return 1;
    }
    else {
        const Group* group = groupFromIndex(parent);
        return group->children().size();
    }
}

int GroupModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    return 1;
}

QModelIndex GroupModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    Group* group;

    if (!parent.isValid()) {
        group = m_root;
    }
    else {
        group = groupFromIndex(parent)->children().at(row);
    }

    return createIndex(row, column, group);
}

QModelIndex GroupModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    return parent(groupFromIndex(index));
}

QModelIndex GroupModel::parent(Group* group) const
{
    Group* parentGroup = group->parentGroup();

    if (!parentGroup) {
        // index is already the root group
        return QModelIndex();
    }
    else {
        const Group* grandParentGroup = parentGroup->parentGroup();
        if (!grandParentGroup) {
            // parent is the root group
            return createIndex(0, 0, parentGroup);
        }
        else {
            return createIndex(grandParentGroup->children().indexOf(parentGroup), 0, parentGroup);
        }
    }
}

QVariant GroupModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    Group* group = groupFromIndex(index);

    if (role == Qt::DisplayRole) {
        return group->name();
    }
    else if (role == Qt::DecorationRole) {
        return group->iconPixmap();
    }
    else {
        return QVariant();
    }
}

QVariant GroupModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    Q_UNUSED(orientation);
    Q_UNUSED(role);

    return QVariant();
}

QModelIndex GroupModel::index(Group* group) const
{
    int row;

    if (!group->parentGroup()) {
        row = 0;
    }
    else {
        row = group->parentGroup()->children().indexOf(group);
    }

    return createIndex(row, 0, group);
}

Group* GroupModel::groupFromIndex(const QModelIndex& index) const
{
    Q_ASSERT(index.internalPointer());

    return static_cast<Group*>(index.internalPointer());
}

void GroupModel::groupDataChanged(Group* group)
{
    QModelIndex ix = index(group);
    Q_EMIT dataChanged(ix, ix);
}

void GroupModel::groupAboutToRemove(Group* group)
{
    Q_ASSERT(group->parentGroup());

    QModelIndex parentIndex = parent(group);
    int pos = group->parentGroup()->children().indexOf(group);

    beginRemoveRows(parentIndex, pos, pos);
}

void GroupModel::groupRemoved()
{
    endRemoveRows();
}

void GroupModel::groupAboutToAdd(Group* group, int index)
{
    Q_ASSERT(group->parentGroup());

    QModelIndex parentIndex = parent(group);

    beginInsertRows(parentIndex, index, index);
}

void GroupModel::groupAdded()
{
    endInsertRows();
}
