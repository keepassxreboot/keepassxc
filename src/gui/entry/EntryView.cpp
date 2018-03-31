/*
 * Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
 * Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or (at your option)
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "EntryView.h"

#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>

#include "core/FilePath.h"
#include "gui/SortFilterHideProxyModel.h"

EntryView::EntryView(QWidget* parent)
    : QTreeView(parent)
    , m_model(new EntryModel(this))
    , m_sortModel(new SortFilterHideProxyModel(this))
    , m_inSearchMode(false)
{
    m_sortModel->setSourceModel(m_model);
    m_sortModel->setDynamicSortFilter(true);
    m_sortModel->setSortLocaleAware(true);
    m_sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    // Use Qt::UserRole as sort role, see EntryModel::data()
    m_sortModel->setSortRole(Qt::UserRole);
    QTreeView::setModel(m_sortModel);

    setUniformRowHeights(true);
    setRootIsDecorated(false);
    setAlternatingRowColors(true);
    setDragEnabled(true);
    setSortingEnabled(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    // QAbstractItemView::startDrag() uses this property as the default drag action
    setDefaultDropAction(Qt::MoveAction);

    connect(this, SIGNAL(doubleClicked(QModelIndex)), SLOT(emitEntryActivated(QModelIndex)));
    connect(
        selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), SIGNAL(entrySelectionChanged()));
    connect(m_model, SIGNAL(switchedToListMode()), SLOT(switchToListMode()));
    connect(m_model, SIGNAL(switchedToSearchMode()), SLOT(switchToSearchMode()));
    connect(m_model, SIGNAL(usernamesHiddenChanged()), SIGNAL(viewStateChanged()));
    connect(m_model, SIGNAL(passwordsHiddenChanged()), SIGNAL(viewStateChanged()));
    connect(this, SIGNAL(clicked(QModelIndex)), SLOT(emitEntryPressed(QModelIndex)));

    m_headerMenu = new QMenu(this);
    m_headerMenu->setTitle(tr("Customize View"));
    m_headerMenu->addSection(tr("Customize View"));

    m_hideUsernamesAction = m_headerMenu->addAction(tr("Hide Usernames"), m_model, SLOT(toggleUsernamesHidden(bool)));
    m_hideUsernamesAction->setCheckable(true);
    m_hidePasswordsAction = m_headerMenu->addAction(tr("Hide Passwords"), m_model, SLOT(togglePasswordsHidden(bool)));
    m_hidePasswordsAction->setCheckable(true);
    m_headerMenu->addSeparator();

    // Actions to toggle column visibility, each carrying the corresponding
    // colummn index as data
    m_columnActions = new QActionGroup(this);
    m_columnActions->setExclusive(false);
    for (int columnIndex = 1; columnIndex < header()->count(); ++columnIndex) {
        QString caption = m_model->headerData(columnIndex, Qt::Horizontal, Qt::DisplayRole).toString();
        if (columnIndex == EntryModel::Paperclip) {
            caption = tr("Attachments (icon)");
        }

        QAction* action = m_headerMenu->addAction(caption);
        action->setCheckable(true);
        action->setData(columnIndex);
        m_columnActions->addAction(action);
    }
    connect(m_columnActions, SIGNAL(triggered(QAction*)), this, SLOT(toggleColumnVisibility(QAction*)));

    m_headerMenu->addSeparator();
    m_headerMenu->addAction(tr("Fit to window"), this, SLOT(fitColumnsToWindow()));
    m_headerMenu->addAction(tr("Fit to contents"), this, SLOT(fitColumnsToContents()));
    m_headerMenu->addSeparator();
    m_headerMenu->addAction(tr("Reset to defaults"), this, SLOT(resetViewToDefaults()));

    header()->setMinimumSectionSize(24);
    header()->setDefaultSectionSize(100);
    header()->setStretchLastSection(false);
    header()->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(header(), SIGNAL(customContextMenuRequested(QPoint)), SLOT(showHeaderMenu(QPoint)));
    connect(header(), SIGNAL(sectionCountChanged(int, int)), SIGNAL(viewStateChanged()));
    connect(header(), SIGNAL(sectionMoved(int, int, int)), SIGNAL(viewStateChanged()));
    connect(header(), SIGNAL(sectionResized(int, int, int)), SIGNAL(viewStateChanged()));
    connect(header(), SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), SIGNAL(viewStateChanged()));

    resetFixedColumns();

    // Configure default search view state and save for later use
    header()->showSection(EntryModel::ParentGroup);
    m_sortModel->sort(EntryModel::ParentGroup, Qt::AscendingOrder);
    sortByColumn(EntryModel::ParentGroup, Qt::AscendingOrder);
    m_defaultSearchViewState = header()->saveState();

    // Configure default list view state and save for later use
    header()->hideSection(EntryModel::ParentGroup);
    m_sortModel->sort(EntryModel::Title, Qt::AscendingOrder);
    sortByColumn(EntryModel::Title, Qt::AscendingOrder);
    m_defaultListViewState = header()->saveState();

    m_model->setPaperClipPixmap(filePath()->icon("actions", "paperclip").pixmap(16));
}

void EntryView::keyPressEvent(QKeyEvent* event)
{
    if ((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) && currentIndex().isValid()) {
        emitEntryActivated(currentIndex());
#ifdef Q_OS_MAC
        // Pressing return does not emit the QTreeView::activated signal on mac os
        emit activated(currentIndex());
#endif
    }

    int last = m_model->rowCount() - 1;

    if (event->key() == Qt::Key_Up && currentIndex().row() == 0) {
        QModelIndex index = m_sortModel->mapToSource(m_sortModel->index(last, 0));
        setCurrentEntry(m_model->entryFromIndex(index));
        return;
    }

    if (event->key() == Qt::Key_Down && currentIndex().row() == last) {
        QModelIndex index = m_sortModel->mapToSource(m_sortModel->index(0, 0));
        setCurrentEntry(m_model->entryFromIndex(index));
        return;
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
    if (m_model->rowCount() > 0) {
        QModelIndex index = m_sortModel->mapToSource(m_sortModel->index(0, 0));
        setCurrentEntry(m_model->entryFromIndex(index));
    } else {
        emit entrySelectionChanged();
    }
}

bool EntryView::inSearchMode()
{
    return m_inSearchMode;
}

void EntryView::emitEntryActivated(const QModelIndex& index)
{
    Entry* entry = entryFromIndex(index);

    emit entryActivated(entry, static_cast<EntryModel::ModelColumn>(m_sortModel->mapToSource(index).column()));
}

void EntryView::emitEntryPressed(const QModelIndex& index)
{
    emit entryPressed(entryFromIndex(index));
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
    } else {
        return nullptr;
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
    } else {
        return nullptr;
    }
}

/**
 * Switch to list mode, i.e. list entries of group
 */
void EntryView::switchToListMode()
{
    if (!m_inSearchMode) {
        return;
    }

    header()->hideSection(EntryModel::ParentGroup);
    m_inSearchMode = false;
}

/**
 * Switch to search mode, i.e. list search results
 */
void EntryView::switchToSearchMode()
{
    if (m_inSearchMode) {
        return;
    }

    header()->showSection(EntryModel::ParentGroup);

    // Always set sorting to column 'Group', as it does not feel right to
    // have the last known sort configuration of search view restored by
    // 'DatabaseWidgetStateSync', which is what happens without this
    m_sortModel->sort(EntryModel::ParentGroup, Qt::AscendingOrder);
    sortByColumn(EntryModel::ParentGroup, Qt::AscendingOrder);

    m_inSearchMode = true;
}

/**
 * Get current state of 'Hide Usernames' setting (NOTE: just pass-through for
 * m_model)
 */
bool EntryView::isUsernamesHidden() const
{
    return m_model->isUsernamesHidden();
}

/**
 * Set state of 'Hide Usernames' setting (NOTE: just pass-through for m_model)
 */
void EntryView::setUsernamesHidden(const bool hide)
{
    m_model->setUsernamesHidden(hide);
}

/**
 * Get current state of 'Hide Passwords' setting (NOTE: just pass-through for
 * m_model)
 */
bool EntryView::isPasswordsHidden() const
{
    return m_model->isPasswordsHidden();
}

/**
 * Set state of 'Hide Passwords' setting (NOTE: just pass-through for m_model)
 */
void EntryView::setPasswordsHidden(const bool hide)
{
    m_model->setPasswordsHidden(hide);
}

/**
 * Get current view state
 */
QByteArray EntryView::viewState() const
{
    return header()->saveState();
}

/**
 * Set view state
 */
bool EntryView::setViewState(const QByteArray& state)
{
    bool status = header()->restoreState(state);
    resetFixedColumns();
    return status;
}

/**
 * Sync checkable menu actions to current state and display header context
 * menu at specified position
 */
void EntryView::showHeaderMenu(const QPoint& position)
{
    m_hideUsernamesAction->setChecked(m_model->isUsernamesHidden());
    m_hidePasswordsAction->setChecked(m_model->isPasswordsHidden());
    const QList<QAction*> actions = m_columnActions->actions();
    for (auto& action : actions) {
        Q_ASSERT(static_cast<QMetaType::Type>(action->data().type()) == QMetaType::Int);
        if (static_cast<QMetaType::Type>(action->data().type()) != QMetaType::Int) {
            continue;
        }
        int columnIndex = action->data().toInt();
        bool hidden = header()->isSectionHidden(columnIndex) || (header()->sectionSize(columnIndex) == 0);
        action->setChecked(!hidden);
    }

    m_headerMenu->popup(mapToGlobal(position));
}

/**
 * Toggle visibility of column referenced by triggering action
 */
void EntryView::toggleColumnVisibility(QAction* action)
{
    // Verify action carries a column index as data. Since QVariant.toInt()
    // below will accept anything that's interpretable as int, perform a type
    // check here to make sure data actually IS int
    Q_ASSERT(static_cast<QMetaType::Type>(action->data().type()) == QMetaType::Int);
    if (static_cast<QMetaType::Type>(action->data().type()) != QMetaType::Int) {
        return;
    }

    // Toggle column visibility. Visible columns will only be hidden if at
    // least one visible column remains, as the table header will disappear
    // entirely when all columns are hidden
    int columnIndex = action->data().toInt();
    if (action->isChecked()) {
        header()->showSection(columnIndex);
        if (header()->sectionSize(columnIndex) == 0) {
            header()->resizeSection(columnIndex, header()->defaultSectionSize());
        }
        return;
    }
    if ((header()->count() - header()->hiddenSectionCount()) > 1) {
        header()->hideSection(columnIndex);
        return;
    }
    action->setChecked(true);
}

/**
 * Resize columns to fit all visible columns within the available space
 *
 * NOTE:
 * If EntryView::resizeEvent() is overridden at some point in the future,
 * its implementation MUST call the corresponding parent method using
 * 'QTreeView::resizeEvent(event)'. Without this, fitting to window will
 * be broken and/or work unreliably (stumbled upon during testing)
 *
 * NOTE:
 * Testing showed that it is absolutely necessary to emit signal 'viewState
 * Changed' here. Without this, an incomplete view state might get saved by
 * 'DatabaseWidgetStateSync' (e.g. only some columns resized)
 */
void EntryView::fitColumnsToWindow()
{
    header()->resizeSections(QHeaderView::Stretch);
    resetFixedColumns();
    fillRemainingWidth(true);
    emit viewStateChanged();
}

/**
 * Resize columns to fit current table contents, i.e. make all contents
 * entirely visible
 */
void EntryView::fitColumnsToContents()
{
    // Resize columns to fit contents
    header()->resizeSections(QHeaderView::ResizeToContents);
    resetFixedColumns();
    fillRemainingWidth(false);
    emit viewStateChanged();
}

/**
 * Reset view to defaults
 */
void EntryView::resetViewToDefaults()
{
    m_model->setUsernamesHidden(false);
    m_model->setPasswordsHidden(true);

    if (m_inSearchMode) {
        header()->restoreState(m_defaultSearchViewState);
    } else {
        header()->restoreState(m_defaultListViewState);
    }

    fitColumnsToWindow();
}

void EntryView::fillRemainingWidth(bool lastColumnOnly)
{
    // Determine total width of currently visible columns
    int width = 0;
    int lastColumnIndex = 0;
    for (int columnIndex = 0; columnIndex < header()->count(); ++columnIndex) {
        if (!header()->isSectionHidden(columnIndex)) {
            width += header()->sectionSize(columnIndex);
        }
        if (header()->visualIndex(columnIndex) > lastColumnIndex) {
            lastColumnIndex = header()->visualIndex(columnIndex);
        }
    }

    int numColumns = header()->count() - header()->hiddenSectionCount();
    int availWidth = header()->width() - width;
    if ((numColumns <= 0) || (availWidth <= 0)) {
        return;
    }

    if (!lastColumnOnly) {
        // Equally distribute remaining width to visible columns
        int add = availWidth / numColumns;
        width = 0;
        for (int columnIndex = 0; columnIndex < header()->count(); ++columnIndex) {
            if (!header()->isSectionHidden(columnIndex)) {
                header()->resizeSection(columnIndex, header()->sectionSize(columnIndex) + add);
                width += header()->sectionSize(columnIndex);
            }
        }
    }

    // Add remaining width to last column
    header()->resizeSection(header()->logicalIndex(lastColumnIndex),
                            header()->sectionSize(lastColumnIndex) + (header()->width() - width));
}

void EntryView::resetFixedColumns()
{
    header()->setSectionResizeMode(EntryModel::Paperclip, QHeaderView::Fixed);
    header()->resizeSection(EntryModel::Paperclip, header()->minimumSectionSize());
}
