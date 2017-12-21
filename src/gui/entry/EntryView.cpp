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
    , m_inEntryListMode(false)
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
    connect(m_model, SIGNAL(switchedToEntryListMode()), SLOT(switchToEntryListMode()));
    connect(m_model, SIGNAL(switchedToGroupMode()), SLOT(switchToGroupMode()));

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
     */
    header()->setDefaultSectionSize(100);
    header()->setStretchLastSection(false);
    header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showHeaderMenu(QPoint)));

    /**
     * @author Fonic <https://github.com/fonic>
     * Finalize setup by resetting view to defaults. Although not really
     * necessary at this point, it makes sense in order to avoid duplicating
     * code (sorting order, visibility of first column etc.)
     *
     * TODO:
     * Not working as expected, columns will end up being very small, most
     * likely due to EntryView not being sized properly at this time. Either
     * find a way to make this work by analizing when/where EntryView is
     * created or remove
     */
    //resetViewToDefaults();
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

bool EntryView::inEntryListMode()
{
    return m_inEntryListMode;
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

void EntryView::switchToEntryListMode()
{
    /**
     * @author Fonic <https://github.com/fonic>
     * Use header()->showSection() instead of m_sortModel->hideColumn() as
     * the latter messes up column indices, interfering with code relying on
     * proper indices
     */
    header()->showSection(EntryModel::ParentGroup);
    if (header()->sectionSize(EntryModel::ParentGroup) == 0) {
        header()->resizeSection(EntryModel::ParentGroup, header()->defaultSectionSize());
    }

    /**
     * @author Fonic <https://github.com/fonic>
     * Set sorting column and order (TODO: check what first two lines do, if
     * they are actually necessary, if indices are still correct and if indices
     * may be replaced by EntryModel::<column>)
     */
    m_sortModel->sort(1, Qt::AscendingOrder);
    m_sortModel->sort(0, Qt::AscendingOrder);
    sortByColumn(EntryModel::ParentGroup, Qt::AscendingOrder);

    m_inEntryListMode = true;
}

void EntryView::switchToGroupMode()
{
    /**
     * @author Fonic <https://github.com/fonic>
     * Use header()->hideSection() instead of m_sortModel->hideColumn() as
     * the latter messes up column indices, interfering with code relying on
     * proper indices
     */
    header()->hideSection(EntryModel::ParentGroup);

    /**
     * @author Fonic <https://github.com/fonic>
     * Set sorting column and order (TODO: check what first two lines do, if
     * they are actually necessary, if indices are still correct and if indices
     * may be replaced by EntryModel::<column>)
     */
    m_sortModel->sort(-1, Qt::AscendingOrder);
    m_sortModel->sort(0, Qt::AscendingOrder);
    sortByColumn(EntryModel::Title, Qt::AscendingOrder);

    m_inEntryListMode = false;
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
 */
void EntryView::fitColumnsToWindow()
{
    header()->resizeSections(QHeaderView::Stretch);
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
}

/**
 * @author Fonic <https://github.com/fonic>
 * Reset view to defaults
 *
 * NOTE:
 * header()->saveState()/restoreState() could also be used for this, but
 * testing showed that it complicates things more than it helps when trying
 * to account for current list mode
 */
void EntryView::resetViewToDefaults()
{
    /* Reset state of 'Hide Usernames'/'Hide Passwords' settings */
    m_model->setHideUsernames(false);
    m_model->setHidePasswords(true);

    /* Reset visibility, size and position of all columns */
    for (int colidx = 0; colidx < header()->count(); colidx++) {
        header()->showSection(colidx);
        header()->resizeSection(colidx, header()->defaultSectionSize());
        header()->moveSection(header()->visualIndex(colidx), colidx);
    }

    /* Reenter current list mode (affects first column and sorting) */
    if (m_inEntryListMode) {
        switchToEntryListMode();
    }
    else {
        switchToGroupMode();
    }

    /* Nicely fitting columns to window feels like a sane default */
    fitColumnsToWindow();
}
