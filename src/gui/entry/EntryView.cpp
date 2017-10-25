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
 * Added includes
 */
#include <QMetaType>

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
    /**
     * @author Fonic <https://github.com/fonic>
     * Set sort role -> 'if (role == Qt::UserRole)', EntryModel::data()
     */
    m_sortModel->setSortRole(Qt::UserRole);
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

    /**
     * @author Fonic <https://github.com/fonic>
     * Header menu:
     * - One checkable action per column to toggle visibility, actions to
     *   resize columns, actions to reset column configuration
     * - Column actions carry the index of 'their' column as data and are
     *   stored in a list to allow syncing their checkboxes to column actual
     *   column visibility -> EntryView::syncColumnActions()
     * - Special column 'Group' (index 0) is excluded (int sec = 1)
     *
     * NOTE:
     * Using 'section' logic of QHeaderView here and in related methods, but
     * there's no real difference between sections and columns in this context
     *
     * TODO:
     * Using old-style QMenu->addAction() for now as Qt version on Travis CI
     * does not feature the new style
     */
    m_headerMenu = new QMenu(this);
    m_headerMenu->setTitle(tr("View"));
    //m_headerMenu->addSection(tr("Display"));
    m_headerMenu->addSection(tr("View"));
    //m_hideUsernamesAction = m_headerMenu->addAction(tr("Hide Usernames"), this, &EntryView::toggleHideUsernames);
    m_hideUsernamesAction = m_headerMenu->addAction(tr("Hide Usernames"), m_model, SLOT(toggleHideUsernames(bool)));
    m_hideUsernamesAction->setCheckable(true);
    m_hideUsernamesAction->setChecked(m_model->hideUsernames());
    //m_hidePasswordsAction = m_headerMenu->addAction(tr("Hide Passwords"), this, &EntryView::toggleHidePasswords);
    m_hidePasswordsAction = m_headerMenu->addAction(tr("Hide Passwords"), m_model, SLOT(toggleHidePasswords(bool)));
    m_hidePasswordsAction->setCheckable(true);
    m_hidePasswordsAction->setChecked(m_model->hidePasswords());
    //m_headerMenu->addSection(tr("Columns"));
    m_headerMenu->addSeparator();
    for (int sec = 1; sec < header()->count(); sec++) {
        QString text = m_model->headerData(sec, Qt::Horizontal, Qt::DisplayRole).toString();
        //QAction *act = m_headerMenu->addAction(text, this, &EntryView::toggleColumnVisibility);
        QAction* act = m_headerMenu->addAction(text, this, SLOT(toggleColumnVisibility()));
        bool hidden = header()->isSectionHidden(sec);
        act->setCheckable(true);
        act->setChecked(!hidden);
        act->setData(sec);
        m_columnActions.append(act);
    }
    m_headerMenu->addSeparator();
    //m_headerMenu->addAction(tr("Fit to window"), this, &EntryView::fitColumnsToWindow);
    //m_headerMenu->addAction(tr("Fit to contents"), this, &EntryView::fitColumnsToContents);
    m_headerMenu->addAction(tr("Fit to window"), this, SLOT(fitColumnsToWindow()));
    m_headerMenu->addAction(tr("Fit to contents"), this, SLOT(fitColumnsToContents()));
    m_headerMenu->addSeparator();
    //m_headerMenu->addAction(tr("Reset to session"), this, &EntryView::resetHeaderToSession);
    //m_headerMenu->addAction(tr("Reset to defaults"), this, &EntryView::resetHeaderToDefaults);
    m_headerMenu->addAction(tr("Reset to session"), this, SLOT(resetHeaderToSession()));
    m_headerMenu->addAction(tr("Reset to defaults"), this, SLOT(resetHeaderToDefaults()));
    header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header(), &QHeaderView::customContextMenuRequested, this, &EntryView::showHeaderMenu);

    /**
     * @author Fonic <https://github.com/fonic>
     * Initialize both default and session state with the current header
     * state. The session state may change when a saved state is applied
     * via EntryView::setHeaderState()
     */
    m_headerSessionState = header()->saveState();
    m_headerDefaultState = header()->saveState();

    connect(this, SIGNAL(doubleClicked(QModelIndex)), SLOT(emitEntryActivated(QModelIndex)));
    connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SIGNAL(entrySelectionChanged()));
    connect(m_model, SIGNAL(switchedToEntryListMode()), SLOT(switchToEntryListMode()));
    connect(m_model, SIGNAL(switchedToGroupMode()), SLOT(switchToGroupMode()));
    /**
     * @author Fonic <https://github.com/fonic>
     * Connect signals to notify about changes of 'Hide Usernames' and 'Hide
     * Passwords' settings of model
     */
    connect(m_model, SIGNAL(hideUsernamesChanged()), SIGNAL(hideUsernamesChanged()));
    connect(m_model, SIGNAL(hidePasswordsChanged()), SIGNAL(hidePasswordsChanged()));
    /**
     * @author Fonic <https://github.com/fonic>
     * Connect signals to notify about header state changes
     */
    connect(header(), SIGNAL(sectionMoved(int, int, int)), SIGNAL(headerStateChanged()));
    connect(header(), SIGNAL(sectionResized(int, int, int)), SIGNAL(headerStateChanged()));
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
 * Called by EntryModel when view enters entry list mode (= list search
 * results)
 *
 * TODO:
 * If header states of list and search mode should be entirely separated,
 * we need to save list state and load search state here
 *
 * NOTE:
 * Using m_sortModel->hideColumn() somehow messes up the column/section
 * indices, most likely due to model and table getting out of sync. Using
 * header()->howSection()/hideSection() instead
 */
void EntryView::switchToEntryListMode()
{
    //m_sortModel->hideColumn(0, false);
    header()->showSection(0);

    m_sortModel->sort(1, Qt::AscendingOrder);
    m_sortModel->sort(0, Qt::AscendingOrder);
    sortByColumn(0, Qt::AscendingOrder);

    m_inEntryListMode = true;
}

/**
 * @author Fonic <https://github.com/fonic>
 * Called by EntryModel when view enters group mode (= list entries of
 * selected group)
 *
 * TODO:
 * If header states of list and search mode should be entirely separated,
 * we need to save search state and load list state here
 *
 * NOTE:
 * Using m_sortModel->hideColumn() somehow messes up the column/section
 * indices, most likely due to model and table getting out of sync. Using
 * header()->howSection()/hideSection() instead
 */
void EntryView::switchToGroupMode()
{
    //m_sortModel->hideColumn(0, true);
    header()->hideSection(0);

    m_sortModel->sort(-1, Qt::AscendingOrder);
    m_sortModel->sort(0, Qt::AscendingOrder);
    sortByColumn(0, Qt::AscendingOrder);

    m_inEntryListMode = false;
}

/**
 * @author Fonic <https://github.com/fonic>
 * Methods to get header state -> used to save configuration to file
 */
QByteArray EntryView::headerState() const
{
    return header()->saveState();
}

/**
 * @author Fonic <https://github.com/fonic>
 * Methods to set header state -> used to load configuration from file
 *
 * TODO BUG(?):
 * Sections are sometimes shown (header()->isSectionHidden() == false),
 * but their size/width is 0, which effectively renders them invisible.
 * We work around this by making sure that invisible sections are also
 * 'officially' hidden
 */
bool EntryView::setHeaderState(const QByteArray &state)
{
    if (header()->restoreState(state)) {
        for (int sec = 1; sec < header()->count(); sec++) {
            if (header()->sectionSize(sec) == 0)
                header()->hideSection(sec);
        }
        m_headerSessionState = header()->saveState();
        syncColumnActions();
        return true;
    }
    else {
        return false;
    }
}

/**
 * @author Fonic <https://github.com/fonic>
 * Get current 'Hide Usernames' setting of model
 */
bool EntryView::hideUsernames() const
{
    return m_model->hideUsernames();
}

/**
 * @author Fonic <https://github.com/fonic>
 * Set 'Hide Usernames' setting of model
 */
void EntryView::setHideUsernames(const bool hide)
{
    m_model->setHideUsernames(hide);
    m_hideUsernamesAction->setChecked(hide);
}

/**
 * @author Fonic <https://github.com/fonic>
 * Get current 'Hide Passwords' setting of model
 */
bool EntryView::hidePasswords() const
{
    return m_model->hidePasswords();
}

/**
 * @author Fonic <https://github.com/fonic>
 * Set 'Hide Passwords' setting of model
 */
void EntryView::setHidePasswords(const bool hide)
{
    m_model->setHidePasswords(hide);
    m_hidePasswordsAction->setChecked(hide);
}

/**
 * @author Fonic <https://github.com/fonic>
 * Display header context menu at current mouse cursor position
 */
void EntryView::showHeaderMenu(const QPoint& pos)
{
    // Could also do a sync here to be on the safe side
    //syncColumnActions();
    m_headerMenu->popup(viewport()->mapToGlobal(pos));
}

/**
 * @author Fonic <https://github.com/fonic>
 * Sync checked state of column actions in header menu to match actual
 * column visibility. Actions not carrying a column index as data are
 * ignored (should never happpen)
 *
 * NOTE: QVariant.canConvert()/toInt() will convert anything that's inter-
 * pretable as an int, so we cast instead to make sure data actually IS
 * an int
 */
void EntryView::syncColumnActions()
{
    foreach (QAction *act, m_columnActions) {
        if (static_cast<QMetaType::Type>(act->data().type()) != QMetaType::Int) {
            Q_ASSERT(false);
            continue;
        }
        int sec = act->data().toInt();
        bool hidden = header()->isSectionHidden(sec);
        act->setChecked(!hidden);
    }
}

/**
 * @author Fonic <https://github.com/fonic>
 * Toggle visibility of a column. This is used as the event handler for
 * 'triggered' signals of column actions in the header menu
 */
void EntryView::toggleColumnVisibility()
{
    // Verify sender of signal is a QAction with a column index as data
    QAction *act = qobject_cast<QAction *>(sender());
    if (!act || (static_cast<QMetaType::Type>(act->data().type()) != QMetaType::Int)) {
        Q_ASSERT(false);
        return;
    }

    // Toggle column visibility
    // - A currently visible column will only be hidden if at least one
    //   visible column remains, since the table header will disappear
    //   entirely when all column are hidden
    // - A currently hidden column will be resized to default size if its
    //   size/width is 0, otherwise it would remain invisible (refer to
    //   comment in EntryView::setHeaderState for details)
    int sec = act->data().toInt();
    if (act->isChecked()) {
        header()->showSection(sec);
        if (header()->sectionSize(sec) == 0)
            //header()->resizeSection(sec, header()->sectionSizeHint(sec));
            header()->resizeSection(sec, header()->defaultSectionSize());
    }
    else {
        if ((header()->count() - header()->hiddenSectionCount()) > 1)
            header()->hideSection(sec);
        else
            act->setChecked(true);
    }
}

/**
 * @author Fonic <https://github.com/fonic>
 * Adjust column sizes to fit all visible columns within the available space
 *
 * TODO BUG:
 * This won't work reliably without explicitely emitting headerStateChanged.
 * Why?
 */
void EntryView::fitColumnsToWindow()
{
    header()->resizeSections(QHeaderView::Stretch);
    emit headerStateChanged();
}

/**
 * @author Fonic <https://github.com/fonic>
 * Adjust column sizes to fit current table contents, i.e. all content is
 * entirely visible
 *
 * TODO BUG:
 * Strangely, in contrast to fitColumnsToWindow(), this DOES work reliably
 * without explicitely emitting headerStateChanged. Why?
 */
void EntryView::fitColumnsToContents()
{
    header()->resizeSections(QHeaderView::ResizeToContents);
    emit headerStateChanged();
}

/**
 * @author Fonic <https://github.com/fonic>
 * Reset column configuration (order, size and visibility) to the state that
 * was in effect when KeePassXC was started (NOTE: state of special column
 * 'Group' is preserved)
 */
void EntryView::resetHeaderToSession()
{
    bool grpcol = header()->isSectionHidden(0);
    header()->restoreState(m_headerSessionState);
    header()->setSectionHidden(0, grpcol);
    syncColumnActions();
}

/**
 * @author Fonic <https://github.com/fonic>
 * Reset column configuration (order, size and visibility) to defaults (NOTE:
 * state of special column 'Group' is preserved)
 */
void EntryView::resetHeaderToDefaults()
{
    bool grpcol = header()->isSectionHidden(0);
    header()->restoreState(m_headerDefaultState);
    header()->setSectionHidden(0, grpcol);
    syncColumnActions();
}
