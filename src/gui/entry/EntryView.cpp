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
/**
 * @author Fonic <https://github.com/fonic>
 * Add include required for header context menu
 */
#include <QMenu>

#include "gui/SortFilterHideProxyModel.h"

/**
 * @author Fonic <https://github.com/fonic>
 *
 * TODO NOTE:
 * Currently, 'zombie' columns which are not hidden but have width == 0
 * (rendering them invisible) may appear. This is caused by DatabaseWidget
 * StateSync. Corresponding checks/workarounds may be removed once sync
 * code is updated accordingly
 * -> relevant code pieces: if (header()->sectionSize(...) == 0) { ... }
 *
 */

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
    /**
     * @author Fonic <https://github.com/fonic>
     * Set Qt::UserRole as sort role
     * -> refer to 'if (role == Qt::UserRole)', EntryModel.cpp, EntryModel::
     *    data() for details
     */
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
    connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SIGNAL(entrySelectionChanged()));
    /**
     * @author Fonic <https://github.com/fonic>
     * Connect signals to get notified about list/search mode switches (NOTE:
     * previously named 'switch[ed]ToGroupMode'/'switch[ed]ToEntryListMode')
     */
    connect(m_model, SIGNAL(switchedToListMode()), SLOT(switchToListMode()));
    connect(m_model, SIGNAL(switchedToSearchMode()), SLOT(switchToSearchMode()));
    /**
     * @author Fonic <https://github.com/fonic>
     * Connect signals to notify about changes of view state when state of
     * 'Hide Usernames'/'Hide Passwords' settings changes in model
     */
    connect(m_model, SIGNAL(hideUsernamesChanged()), SIGNAL(viewStateChanged()));
    connect(m_model, SIGNAL(hidePasswordsChanged()), SIGNAL(viewStateChanged()));

    connect(this, SIGNAL(clicked(QModelIndex)), SLOT(emitEntryPressed(QModelIndex)));

    /**
     * @author Fonic <https://github.com/fonic>
     * Create header context menu:
     * - Actions to toggle state of 'Hide Usernames'/'Hide Passwords' settings
     * - Actions to toggle column visibility, with each action carrying 'its'
     *   column index as data
     * - Actions to resize columns
     * - Action to reset view to defaults
     */
    m_headerMenu = new QMenu(this);
    m_headerMenu->setTitle(tr("Customize View"));
    m_headerMenu->addSection(tr("Customize View"));

    m_hideUsernamesAction = m_headerMenu->addAction(tr("Hide Usernames"), m_model, SLOT(toggleHideUsernames(bool)));
    m_hideUsernamesAction->setCheckable(true);
    m_hidePasswordsAction = m_headerMenu->addAction(tr("Hide Passwords"), m_model, SLOT(toggleHidePasswords(bool)));
    m_hidePasswordsAction->setCheckable(true);
    m_headerMenu->addSeparator();

    m_columnActions = new QActionGroup(this);
    m_columnActions->setExclusive(false);
    for (int colidx = 1; colidx < header()->count(); colidx++) {
        QString caption = m_model->headerData(colidx, Qt::Horizontal, Qt::DisplayRole).toString();
        QAction* action = m_headerMenu->addAction(caption);
        action->setCheckable(true);
        action->setData(colidx);
        m_columnActions->addAction(action);
    }
    connect(m_columnActions, SIGNAL(triggered(QAction*)), this, SLOT(toggleColumnVisibility(QAction*)));

    m_headerMenu->addSeparator();
    m_headerMenu->addAction(tr("Fit to window"), this, SLOT(fitColumnsToWindow()));
    m_headerMenu->addAction(tr("Fit to contents"), this, SLOT(fitColumnsToContents()));
    m_headerMenu->addSeparator();
    m_headerMenu->addAction(tr("Reset to defaults"), this, SLOT(resetViewToDefaults()));

    /**
     * @author Fonic <https://github.com/fonic>
     * Configure header:
     * - Set default section size
     * - Disable stretching of last section (interferes with fitting columns
     *   to window)
     * - Associate with context menu
     * - Connect signals to notify about changes of view state when state
     *   of header changes
     */
    header()->setDefaultSectionSize(100);
    header()->setStretchLastSection(false);
    header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showHeaderMenu(QPoint)));
    connect(header(), SIGNAL(sectionCountChanged(int, int)), this, SIGNAL(viewStateChanged()));
    connect(header(), SIGNAL(sectionMoved(int, int, int)), this, SIGNAL(viewStateChanged()));
    connect(header(), SIGNAL(sectionResized(int, int, int)), this, SIGNAL(viewStateChanged()));
    connect(header(), SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), this, SIGNAL(viewStateChanged()));

    /**
     * @author Fonic <https://github.com/fonic>
     * Fit columns to window
     *
     * TODO:
     * Not working as expected, columns will end up being very small, most
     * likely due to EntryView not being sized properly at this time. Find
     * a way to make this work by analizing when/where EntryView is created
     */
    //fitColumnsToWindow();

    /**
     * @author Fonic <https://github.com/fonic>
     * Configure default search view state and save for later use
     */
    header()->showSection(EntryModel::ParentGroup);
    m_sortModel->sort(EntryModel::ParentGroup, Qt::AscendingOrder);
    sortByColumn(EntryModel::ParentGroup, Qt::AscendingOrder);
    m_defaultSearchViewState = header()->saveState();

    /**
     * @author Fonic <https://github.com/fonic>
     * Configure default list view state and save for later use
     *
     * NOTE:
     * Default list view is intentionally configured last since this is the
     * view that's supposed to be active after initialization as m_inSearchMode
     * is initialized with 'false'
     */
    header()->hideSection(EntryModel::ParentGroup);
    m_sortModel->sort(EntryModel::Title, Qt::AscendingOrder);
    sortByColumn(EntryModel::Title, Qt::AscendingOrder);
    m_defaultListViewState = header()->saveState();
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
    }
    else {
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
    }
    else {
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
    }
    else {
        return nullptr;
    }
}

/**
 * @author Fonic <https://github.com/fonic>
 * Switch to list mode, i.e. list entries of group (NOTE: previously named
 * 'switchToGroupMode')
 */
void EntryView::switchToListMode()
{
    /* Check if already in this mode */
    if (!m_inSearchMode) {
        return;
    }

    /**
     * Use header()->hideSection() instead of m_sortModel->hideColumn() as
     * the latter messes up column indices, interfering with code relying on
     * proper indices
     */
    header()->hideSection(EntryModel::ParentGroup);

    m_inSearchMode = false;
}

/**
 * @author Fonic <https://github.com/fonic>
 * Switch to search mode, i.e. list search results (NOTE: previously named
 * 'switchToEntryListMode')
 */
void EntryView::switchToSearchMode()
{
    /* Check if already in this mode */
    if (m_inSearchMode) {
        return;
    }

    /*
     * Use header()->showSection() instead of m_sortModel->hideColumn() as
     * the latter messes up column indices, interfering with code relying on
     * proper indices
     */
    header()->showSection(EntryModel::ParentGroup);

    /*
     * Always set sorting to column 'Group', as it does not feel right to have
     * the last known sort configuration of search view restored by 'Database
     * WidgetStateSync', which is what happens without this
     */
    m_sortModel->sort(EntryModel::ParentGroup, Qt::AscendingOrder);
    sortByColumn(EntryModel::ParentGroup, Qt::AscendingOrder);

    m_inSearchMode = true;
}

/**
 * @author Fonic <https://github.com/fonic>
 * Get current state of 'Hide Usernames' setting (NOTE: just pass-through for
 * m_model)
 */
bool EntryView::hideUsernames() const
{
    return m_model->hideUsernames();
}

/**
 * @author Fonic <https://github.com/fonic>
 * Set state of 'Hide Usernames' setting (NOTE: just pass-through for m_model)
 */
void EntryView::setHideUsernames(const bool hide)
{
    m_model->setHideUsernames(hide);
}

/**
 * @author Fonic <https://github.com/fonic>
 * Get current state of 'Hide Passwords' setting (NOTE: just pass-through for
 * m_model)
 */
bool EntryView::hidePasswords() const
{
    return m_model->hidePasswords();
}

/**
 * @author Fonic <https://github.com/fonic>
 * Set state of 'Hide Passwords' setting (NOTE: just pass-through for m_model)
 */
void EntryView::setHidePasswords(const bool hide)
{
    m_model->setHidePasswords(hide);
}

/**
 * @author Fonic <https://github.com/fonic>
 * Get current state of view
 */
QByteArray EntryView::viewState() const
{
    return header()->saveState();
}

/**
 * @author Fonic <https://github.com/fonic>
 * Set state of entry view view
 */
bool EntryView::setViewState(const QByteArray& state) const
{
    return header()->restoreState(state);
}

/**
 * @author Fonic <https://github.com/fonic>
 * Sync checkable menu actions to current state and display header context
 * menu at specified position
 */
void EntryView::showHeaderMenu(const QPoint& position)
{
    /* Sync checked state of menu actions to current state of view */
    m_hideUsernamesAction->setChecked(m_model->hideUsernames());
    m_hidePasswordsAction->setChecked(m_model->hidePasswords());
    foreach (QAction *action, m_columnActions->actions()) {
        if (static_cast<QMetaType::Type>(action->data().type()) != QMetaType::Int) {
            Q_ASSERT(false);
            continue;
        }
        int colidx = action->data().toInt();
        bool hidden = header()->isSectionHidden(colidx) || (header()->sectionSize(colidx) == 0);
        action->setChecked(!hidden);
    }

    /* Display menu */
    m_headerMenu->popup(mapToGlobal(position));
}

/**
 * @author Fonic <https://github.com/fonic>
 * Toggle visibility of column referenced by triggering action
 */
void EntryView::toggleColumnVisibility(QAction *action)
{
    /*
     * Verify action carries a column index as data. Since QVariant.toInt()
     * below will accept anything that's interpretable as int, perform a type
     * check here to make sure data actually IS int
     */
    if (static_cast<QMetaType::Type>(action->data().type()) != QMetaType::Int) {
        Q_ASSERT(false);
        return;
    }

    /*
     * Toggle column visibility. Visible columns will only be hidden if at
     * least one visible column remains, as the table header will disappear
     * entirely when all columns are hidden, rendering the context menu in-
     * accessible
     */
    int colidx = action->data().toInt();
    if (action->isChecked()) {
        header()->showSection(colidx);
        if (header()->sectionSize(colidx) == 0) {
            header()->resizeSection(colidx, header()->defaultSectionSize());
        }
    }
    else {
        if ((header()->count() - header()->hiddenSectionCount()) > 1) {
            header()->hideSection(colidx);
        }
        else {
            action->setChecked(true);
        }
    }
}

/**
 * @author Fonic <https://github.com/fonic>
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
    emit viewStateChanged();
}

/**
 * @author Fonic <https://github.com/fonic>
 * Resize columns to fit current table contents, i.e. make all contents
 * entirely visible
 */
void EntryView::fitColumnsToContents()
{
    /* Resize columns to fit contents */
    header()->resizeSections(QHeaderView::ResizeToContents);

    /*
     * Determine total width of currently visible columns. If there is
     * still some space available on the header, equally distribute it to
     * visible columns and add remaining fraction to last visible column
     */
    int width = 0;
    for (int colidx = 0; colidx < header()->count(); colidx++) {
        if (!header()->isSectionHidden(colidx)) {
            width += header()->sectionSize(colidx);
        }
    }
    int visible = header()->count() - header()->hiddenSectionCount();
    int avail = header()->width() - width;
    if ((visible > 0) && (avail > 0)) {
        int add = avail / visible;
        width = 0;
        int last = 0;
        for (int colidx = 0; colidx < header()->count(); colidx++) {
            if (!header()->isSectionHidden(colidx)) {
                header()->resizeSection(colidx, header()->sectionSize(colidx) + add);
                width += header()->sectionSize(colidx);
                if (header()->visualIndex(colidx) > last) {
                    last = header()->visualIndex(colidx);
                }
            }
        }
        header()->resizeSection(header()->logicalIndex(last), header()->sectionSize(last) + (header()->width() - width));
    }

    /*
     * This should not be necessary due to use of header()->resizeSection,
     * but lets do it anyway for the sake of completeness
     */
    emit viewStateChanged();
}

/**
 * @author Fonic <https://github.com/fonic>
 * Reset view to defaults
 */
void EntryView::resetViewToDefaults()
{
    /* Reset state of 'Hide Usernames'/'Hide Passwords' settings */
    m_model->setHideUsernames(false);
    m_model->setHidePasswords(true);

    /* Reset columns (size, order, sorting etc.) */
    if (m_inSearchMode) {
        header()->restoreState(m_defaultSearchViewState);
    }
    else {
        header()->restoreState(m_defaultListViewState);
    }

    /* Nicely fitting columns to window feels like a sane default */
    fitColumnsToWindow();
}
