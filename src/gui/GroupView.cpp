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

#include <QtCore/QMetaObject>

#include "core/Database.h"
#include "core/Group.h"
#include "gui/GroupModel.h"

GroupView::GroupView(Database* db, QWidget* parent)
    : QTreeView(parent)
{
    m_model = new GroupModel(db, this);
    QTreeView::setModel(m_model);
    setHeaderHidden(true);
    setUniformRowHeights(true);

    connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(emitGroupChanged()));

    recInitExpanded(db->rootGroup());

    setCurrentIndex(m_model->index(0, 0));
    // invoke later so the EntryView is connected
    QMetaObject::invokeMethod(this, "emitGroupChanged", Qt::QueuedConnection,
                              Q_ARG(QModelIndex, m_model->index(0, 0)));
}

Group* GroupView::currentGroup()
{
    return m_model->groupFromIndex(currentIndex());
}

void GroupView::expandedChanged(const QModelIndex& index)
{
    Group* group = m_model->groupFromIndex(index);
    group->setExpanded(isExpanded(index));
}

void GroupView::recInitExpanded(Group* group)
{
    QModelIndex index = m_model->index(group);
    setExpanded(index, group->isExpanded());

    Q_FOREACH (Group* child, group->children()) {
        recInitExpanded(child);
    }
}

void GroupView::emitGroupChanged(const QModelIndex& index)
{
    Q_EMIT groupChanged(m_model->groupFromIndex(index));
}

void GroupView::setModel(QAbstractItemModel* model)
{
    Q_UNUSED(model);
    Q_ASSERT(false);
}

void GroupView::emitGroupChanged()
{
    Q_EMIT groupChanged(currentGroup());
}
