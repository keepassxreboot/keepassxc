/*
 *  Copyright (C) 2015 David Wu <lightvector@gmail.com>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "AutoTypeMatchView.h"

#include <QHeaderView>
#include <QKeyEvent>

#include "gui/SortFilterHideProxyModel.h"

AutoTypeMatchView::AutoTypeMatchView(QWidget* parent)
    : QTreeView(parent)
    , m_model(new AutoTypeMatchModel(this))
    , m_sortModel(new SortFilterHideProxyModel(this))
{
    m_sortModel->setSourceModel(m_model);
    m_sortModel->setDynamicSortFilter(true);
    m_sortModel->setSortLocaleAware(true);
    m_sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    QTreeView::setModel(m_sortModel);

    setUniformRowHeights(true);
    setRootIsDecorated(false);
    setAlternatingRowColors(true);
    setDragEnabled(false);
    setSortingEnabled(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    header()->setDefaultSectionSize(150);

    connect(this, SIGNAL(doubleClicked(QModelIndex)), SLOT(emitMatchActivated(QModelIndex)));
    connect(
        selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), SIGNAL(matchSelectionChanged()));
}

void AutoTypeMatchView::keyPressEvent(QKeyEvent* event)
{
    if ((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) && currentIndex().isValid()) {
        emitMatchActivated(currentIndex());
#ifdef Q_OS_MAC
        // Pressing return does not emit the QTreeView::activated signal on mac os
        emit activated(currentIndex());
#endif
    }

    QTreeView::keyPressEvent(event);
}

void AutoTypeMatchView::setMatchList(const QList<AutoTypeMatch>& matches)
{
    m_model->setMatchList(matches);
    for (int i = 0; i < m_model->columnCount(); ++i) {
        resizeColumnToContents(i);
        if (columnWidth(i) > 250) {
            setColumnWidth(i, 250);
        }
    }
    setFirstMatchActive();
}

void AutoTypeMatchView::setFirstMatchActive()
{
    if (m_model->rowCount() > 0) {
        QModelIndex index = m_sortModel->mapToSource(m_sortModel->index(0, 0));
        setCurrentMatch(m_model->matchFromIndex(index));
    } else {
        emit matchSelectionChanged();
    }
}

void AutoTypeMatchView::emitMatchActivated(const QModelIndex& index)
{
    AutoTypeMatch match = matchFromIndex(index);

    emit matchActivated(match);
}

AutoTypeMatch AutoTypeMatchView::currentMatch()
{
    QModelIndexList list = selectionModel()->selectedRows();
    if (list.size() == 1) {
        return m_model->matchFromIndex(m_sortModel->mapToSource(list.first()));
    }
    return AutoTypeMatch();
}

void AutoTypeMatchView::setCurrentMatch(AutoTypeMatch match)
{
    selectionModel()->setCurrentIndex(m_sortModel->mapFromSource(m_model->indexFromMatch(match)),
                                      QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

AutoTypeMatch AutoTypeMatchView::matchFromIndex(const QModelIndex& index)
{
    if (index.isValid()) {
        return m_model->matchFromIndex(m_sortModel->mapToSource(index));
    }
    return AutoTypeMatch();
}
