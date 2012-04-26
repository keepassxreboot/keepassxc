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

#include <QtCore/QMimeData>

#include "core/Database.h"
#include "core/Group.h"
#include "core/Tools.h"

GroupModel::GroupModel(Database* db, QObject* parent)
    : QAbstractItemModel(parent)
{
    m_root = db->rootGroup();
    connect(db, SIGNAL(groupDataChanged(Group*)), SLOT(groupDataChanged(Group*)));
    connect(db, SIGNAL(groupAboutToAdd(Group*,int)), SLOT(groupAboutToAdd(Group*,int)));
    connect(db, SIGNAL(groupAdded()), SLOT(groupAdded()));
    connect(db, SIGNAL(groupAboutToRemove(Group*)), SLOT(groupAboutToRemove(Group*)));
    connect(db, SIGNAL(groupRemoved()), SLOT(groupRemoved()));
    connect(db, SIGNAL(groupAboutToMove(Group*,Group*,int)), SLOT(groupAboutToMove(Group*,Group*,int)));
    connect(db, SIGNAL(groupMoved()), SLOT(groupMoved()));
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

Qt::DropActions GroupModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

Qt::ItemFlags GroupModel::flags(const QModelIndex& modelIndex) const
{
    if (!modelIndex.isValid()) {
        return Qt::NoItemFlags;
    }
    else if (modelIndex == index(0, 0)) {
        return QAbstractItemModel::flags(modelIndex) | Qt::ItemIsDropEnabled;
    }
    else {
        return QAbstractItemModel::flags(modelIndex) | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled;
    }
}

bool GroupModel::dropMimeData(const QMimeData* data, Qt::DropAction action,
                              int row, int column, const QModelIndex& parent)
{
    Q_UNUSED(column);

    if (!data || (action != Qt::MoveAction) || !parent.isValid()) {
        return false;
    }

    // check if the format is supported
    QStringList types = mimeTypes();
    Q_ASSERT(types.size() == 2);
    bool isGroup = data->hasFormat(types.at(0));
    bool isEntry = data->hasFormat(types.at(1));
    if (!isGroup && !isEntry) {
        return false;
    }

    if (row > rowCount(parent)) {
        row = rowCount(parent);
    }

    // decode and insert
    QByteArray encoded = data->data(isGroup ? types.at(0) : types.at(1));
    QDataStream stream(&encoded, QIODevice::ReadOnly);
    Uuid dbUuid;
    Uuid itemUuid;
    stream >> dbUuid >> itemUuid;

    Database* db = Database::databaseByUuid(dbUuid);
    if (!db) {
        return false;
    }

    Group* parentGroup = groupFromIndex(parent);

    if (isGroup) {
        Group* dragGroup = db->resolveGroup(itemUuid);
        if (!dragGroup || !Tools::hasChild(db, dragGroup) || dragGroup == db->rootGroup()) {
            return false;
        }

        if (dragGroup == parentGroup || Tools::hasChild(dragGroup, parentGroup)) {
            return false;
        }

        if (parentGroup == dragGroup->parent() && row > parentGroup->children().indexOf(dragGroup)) {
            row--;
        }

        dragGroup->setParent(parentGroup, row);
    }
    else {
        Entry* dragEntry = db->resolveEntry(itemUuid);
        if (!dragEntry || !Tools::hasChild(db, dragEntry) || row != -1) {
            return false;
        }

        dragEntry->setGroup(parentGroup);
    }

    return true;
}

QStringList GroupModel::mimeTypes() const
{
    QStringList types;
    types << "application/x-keepassx-group";
    types << "application/x-keepassx-entry";
    return types;
}

QMimeData* GroupModel::mimeData(const QModelIndexList& indexes) const
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
        stream << m_root->database()->uuid() << groupFromIndex(indexes[i])->uuid();
    }

    data->setData(mimeTypes().first(), encoded);
    return data;
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

void GroupModel::groupAboutToMove(Group* group, Group* toGroup, int pos)
{
    Q_ASSERT(group->parentGroup());

    QModelIndex oldParentIndex = parent(group);
    QModelIndex newParentIndex = index(toGroup);
    int oldPos = group->parentGroup()->children().indexOf(group);
    if (group->parentGroup() == toGroup && pos > oldPos) {
        // beginMoveRows() has a bit different semantics than Group::setParent() and
        // QList::move() when the new position is greater than the old
        pos++;
    }

    bool moveResult = beginMoveRows(oldParentIndex, oldPos, oldPos, newParentIndex, pos);
    Q_UNUSED(moveResult);
    Q_ASSERT(moveResult);
}

void GroupModel::groupMoved()
{
    endMoveRows();
}
