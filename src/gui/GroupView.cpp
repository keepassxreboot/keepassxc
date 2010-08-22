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

#include "GroupView.h"

#include "core/Database.h"
#include "core/Group.h"
#include "gui/GroupModel.h"

GroupView::GroupView(Database* db, QWidget* parent) : QTreeView(parent)
{
    model = new GroupModel(db, this);
    QTreeView::setModel(model);
    recInitExpanded(db->rootGroup());
    setHeaderHidden(true);
}

void GroupView::expandedChanged(const QModelIndex& index)
{
    Group* group = const_cast<Group*>(model->groupFromIndex(index));
    group->setExpanded(isExpanded(index));
}

void GroupView::recInitExpanded(const Group* group)
{
    QModelIndex index = model->index(group);
    setExpanded(index, group->isExpanded());

    Q_FOREACH (const Group* child, group->children()) {
        recInitExpanded(child);
    }
}

void GroupView::setModel(QAbstractItemModel* model)
{
    Q_UNUSED(model);
    Q_ASSERT(false);
}
