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

#include "core/Group.h"

GroupModel::GroupModel(const Group* rootGroup, QObject* parent) : QAbstractItemModel(parent)
{
    m_root = rootGroup;
}

void GroupModel::setRootGroup(const Group* group)
{
    m_root = group;
}

int GroupModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
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

    const Group* group;

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

    const Group* childGroup = groupFromIndex(index);
    const Group* parentGroup = childGroup->parentGroup();

    if (!parentGroup) {
        return QModelIndex();
    }
    else {
        const Group* grandParentGroup = parentGroup->parentGroup();
        if (!grandParentGroup) {
            return createIndex(0, 0, parentGroup);
        }
        else {
            return createIndex(grandParentGroup->children().indexOf(parentGroup), 0, parentGroup);
        }
    }
}

QVariant GroupModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    const Group* group = groupFromIndex(index);
    return group->name();
}

QVariant GroupModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    Q_UNUSED(orientation);
    Q_UNUSED(role);

    return QVariant();
}

QModelIndex GroupModel::createIndex(int row, int column, const Group* group) const
{
    return QAbstractItemModel::createIndex(row, column, const_cast<Group*>(group));
}

const Group* GroupModel::groupFromIndex(const QModelIndex& index) const
{
    Q_ASSERT(index.internalPointer());

    return static_cast<const Group*>(index.internalPointer());
}
