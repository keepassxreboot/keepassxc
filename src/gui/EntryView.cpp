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

#include <QtGui/QSortFilterProxyModel>

#include "gui/EntryModel.h"

EntryView::EntryView(QWidget* parent)
    : QTreeView(parent)
    , m_model(new EntryModel(this))
    , m_sortModel(new QSortFilterProxyModel(this))
{
    m_sortModel->setSourceModel(m_model);
    m_sortModel->setDynamicSortFilter(true);
    m_sortModel->setSortLocaleAware(true);
    m_sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_sortModel->setSupportedDragActions(m_model->supportedDragActions());
    QTreeView::setModel(m_sortModel);

    setUniformRowHeights(true);
    setRootIsDecorated(false);
    setDragEnabled(true);
    setSortingEnabled(true);

    connect(this, SIGNAL(activated(const QModelIndex&)), SLOT(emitEntryActivated(const QModelIndex&)));
    connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SIGNAL(entrySelectionChanged()));
    connect(m_model, SIGNAL(switchedToSearch()), this, SLOT(switchToSearch()));
    connect(m_model, SIGNAL(switchedToView()), this, SLOT(switchToView()));

    sortByColumn(0, Qt::AscendingOrder);
}

void EntryView::setGroup(Group* group)
{
    m_model->setGroup(group);
    Q_EMIT entrySelectionChanged();
}

void EntryView::search(QList<Entry*> entries)
{
    m_model->setEntries(entries);
    Q_EMIT entrySelectionChanged();
}

void EntryView::emitEntryActivated(const QModelIndex& index)
{
    Q_EMIT entryActivated(entryFromIndex(index));
}

void EntryView::setModel(QAbstractItemModel* model)
{
    Q_UNUSED(model);
    Q_ASSERT(false);
}

Entry* EntryView::currentEntry()
{
    // TODO: use selection instead of current?
    return m_model->entryFromIndex(m_sortModel->mapToSource(currentIndex()));
}

bool EntryView::isSingleEntrySelected()
{
    return (selectionModel()->selectedRows().size() == 1);
}

void EntryView::setCurrentEntry(Entry* entry)
{
    setCurrentIndex(m_sortModel->mapFromSource(m_model->indexFromEntry(entry)));
}

Entry* EntryView::entryFromIndex(const QModelIndex& index)
{
    if (index.isValid()) {
        return m_model->entryFromIndex(m_sortModel->mapToSource(index));
    }
    else {
        return 0;
    }
}

void EntryView::switchToSearch()
{
    showColumn(0);
}

void EntryView::switchToView()
{
    hideColumn(0);
}
