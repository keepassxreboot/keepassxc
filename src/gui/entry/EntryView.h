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

#ifndef KEEPASSX_ENTRYVIEW_H
#define KEEPASSX_ENTRYVIEW_H

#include <QTreeView>

#include "gui/entry/EntryModel.h"

class Entry;
class EntryModel;
class Group;
class SortFilterHideProxyModel;
/**
 * @author Fonic <https://github.com/fonic>
 * Add forward declaration for QActionGroup
 */
class QActionGroup;

class EntryView : public QTreeView
{
    Q_OBJECT

public:
    explicit EntryView(QWidget* parent = nullptr);
    void setModel(QAbstractItemModel* model) override;
    Entry* currentEntry();
    void setCurrentEntry(Entry* entry);
    Entry* entryFromIndex(const QModelIndex& index);
    void setEntryList(const QList<Entry*>& entries);
    bool inSearchMode();
    int numberOfSelectedEntries();
    void setFirstEntryActive();

    /**
     * @author Fonic <https://github.com/fonic>
     * Methods to get/set state of 'Hide Usernames'/'Hide Passwords' settings
     * (NOTE: these are just pass-through methods to avoid exposing entry model)
     */
    bool hideUsernames() const;
    void setHideUsernames(const bool hide);
    bool hidePasswords() const;
    void setHidePasswords(const bool hide);

    /**
     * @author Fonic <https://github.com/fonic>
     * Methods to get/set state of view
     */
    QByteArray viewState() const;
    bool setViewState(const QByteArray& state) const;

public slots:
    void setGroup(Group* group);

signals:
    void entryActivated(Entry* entry, EntryModel::ModelColumn column);
    void entryPressed(Entry* entry);
    void entrySelectionChanged();
    /**
     * @author Fonic <https://github.com/fonic>
     * Signal to notify about changes of view state
     */
    void viewStateChanged();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void emitEntryActivated(const QModelIndex& index);
    void emitEntryPressed(const QModelIndex& index);
    /**
     * @author Fonic <https://github.com/fonic>
     * Methods to switch to list/search mode (NOTE: previously named 'switch
     * ToGroupMode'/'switchToEntryListMode')
     */
    void switchToListMode();
    void switchToSearchMode();

    /**
     * @author Fonic <https://github.com/fonic>
     * Slots for header context menu and actions
     */
    void showHeaderMenu(const QPoint& position);
    void toggleColumnVisibility(QAction *action);
    void fitColumnsToWindow();
    void fitColumnsToContents();
    void resetViewToDefaults();

private:
    EntryModel* const m_model;
    SortFilterHideProxyModel* const m_sortModel;
    bool m_inSearchMode;

    /**
     * @author Fonic <https://github.com/fonic>
     * Properties to store default view states used by resetViewToDefaults()
     */
    QByteArray m_defaultListViewState;
    QByteArray m_defaultSearchViewState;
    /**
     * @author Fonic <https://github.com/fonic>
     * Properties to store header context menu and actions
     */
    QMenu* m_headerMenu;
    QAction* m_hideUsernamesAction;
    QAction* m_hidePasswordsAction;
    QActionGroup* m_columnActions;
};

#endif // KEEPASSX_ENTRYVIEW_H
