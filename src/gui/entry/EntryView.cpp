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

#include "EntryView.h"

#include <QHeaderView>
#include <QKeyEvent>

#include "gui/SortFilterHideProxyModel.h"

EntryView::EntryView(QWidget* parent)
    : QTreeView(parent)
    , m_model(new EntryModel(this))
    , m_sortModel(new SortFilterHideProxyModel(this))
    , m_inEntryListMode(false)
{
    m_sortModel->setSourceModel(m_model);
    m_sortModel->setDynamicSortFilter(true);
    m_sortModel->setSortLocaleAware(true);
    m_sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_sortModel->setSupportedDragActions(m_model->supportedDragActions());
    QTreeView::setModel(m_sortModel);

    setUniformRowHeights(true);
    setRootIsDecorated(false);
    setAlternatingRowColors(true);
    setDragEnabled(true);
    setSortingEnabled(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    header()->setDefaultSectionSize(150);

    // QAbstractItemView::startDrag() uses this property as the default drag action
    setDefaultDropAction(Qt::MoveAction);

    connect(this, SIGNAL(doubleClicked(QModelIndex)), SLOT(emitEntryActivated(QModelIndex)));
    connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SIGNAL(entrySelectionChanged()));
    connect(m_model, SIGNAL(switchedToEntryListMode()), SLOT(switchToEntryListMode()));
    connect(m_model, SIGNAL(switchedToGroupMode()), SLOT(switchToGroupMode()));
}

void EntryView::keyPressEvent(QKeyEvent* event)
{
    if ((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) && currentIndex().isValid()) {
        emitEntryActivated(currentIndex());
    }

    QTreeView::keyPressEvent(event);
}

void EntryView::setGroup(Group* group)
{
    m_model->setGroup(group);
    setFirstEntryActive();
}

void EntryView::setEntryList(const QList<Entry*>& entries)
{
    m_model->setEntryList(entries);
    setFirstEntryActive();
}

void EntryView::setFirstEntryActive()
{
    if(m_model->rowCount() > 0) {
        QModelIndex index = m_sortModel->mapToSource(m_sortModel->index(0, 0));
        setCurrentEntry(m_model->entryFromIndex(index));
    }
    else {
        Q_EMIT entrySelectionChanged();
    }
}

bool EntryView::inEntryListMode()
{
    return m_inEntryListMode;
}

void EntryView::emitEntryActivated(const QModelIndex& index)
{
    Entry* entry = entryFromIndex(index);

    Q_EMIT entryActivated(entry, static_cast<EntryModel::ModelColumn>(m_sortModel->mapToSource(index).column()));
}

void EntryView::setModel(QAbstractItemModel* model)
{
    Q_UNUSED(model);
    Q_ASSERT(false);
}

Entry* EntryView::currentEntry()
{
    QModelIndexList list = selectionModel()->selectedRows();
    if (list.size() == 1) {
        return m_model->entryFromIndex(m_sortModel->mapToSource(list.first()));
    }
    else {
        return Q_NULLPTR;
    }
}

int EntryView::numberOfSelectedEntries()
{
    return selectionModel()->selectedRows().size();
}

void EntryView::setCurrentEntry(Entry* entry)
{
    selectionModel()->setCurrentIndex(m_sortModel->mapFromSource(m_model->indexFromEntry(entry)),
                                      QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

Entry* EntryView::entryFromIndex(const QModelIndex& index)
{
    if (index.isValid()) {
        return m_model->entryFromIndex(m_sortModel->mapToSource(index));
    }
    else {
        return Q_NULLPTR;
    }
}

void EntryView::switchToEntryListMode()
{
    m_sortModel->hideColumn(0, false);
    sortByColumn(1, Qt::AscendingOrder); // TODO: should probably be improved
    sortByColumn(0, Qt::AscendingOrder);
    m_inEntryListMode = true;
}

void EntryView::switchToGroupMode()
{
    m_sortModel->hideColumn(0, true);
    sortByColumn(-1, Qt::AscendingOrder);
    sortByColumn(0, Qt::AscendingOrder);
    m_inEntryListMode = false;
}
