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
/**
 * @author Fonic <https://github.com/fonic>
 * Added includes
 */
#include <QMenu>
#include <QAction>
#include <QPoint>
#include <QByteArray>

#include "gui/entry/EntryModel.h"

class Entry;
class EntryModel;
class Group;
class SortFilterHideProxyModel;

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
    bool inEntryListMode();
    int numberOfSelectedEntries();
    void setFirstEntryActive();
    /**
     * @author Fonic <https://github.com/fonic>
     * Methods to get/set header state -> used by DatabaseWidget to save/load
     * configuration to/from file
     */
    QByteArray headerState() const;
    bool setHeaderState(const QByteArray &state);
    /**
     * @author Fonic <https://github.com/fonic>
     * Methods to get/set 'Hide Usernames' and 'Hide Passwords' settings
     * of model
     */
    bool hideUsernames() const;
    void setHideUsernames(const bool hide);
    bool hidePasswords() const;
    void setHidePasswords(const bool hide);

public slots:
    void setGroup(Group* group);

signals:
    void entryActivated(Entry* entry, EntryModel::ModelColumn column);
    void entrySelectionChanged();
    /**
     * @author Fonic <https://github.com/fonic>
     * Signal to notify about header state changes
     */
    void headerStateChanged();
    /**
     * @author Fonic <https://github.com/fonic>
     * Signals to notify about changes of 'Hide Usernames' and 'Hide Passwords'
     * settings of model
     */
    void hideUsernamesChanged();
    void hidePasswordsChanged();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void emitEntryActivated(const QModelIndex& index);
    void switchToEntryListMode();
    void switchToGroupMode();
    /**
     * @author Fonic <https://github.com/fonic>
     * Signal slots for header menu
     */
    void showHeaderMenu(const QPoint& pos);
    void syncColumnActions();
    void toggleColumnVisibility();
    void fitColumnsToWindow();
    void fitColumnsToContents();
    void resetHeaderToSession();
    void resetHeaderToDefaults();

private:
    EntryModel* const m_model;
    SortFilterHideProxyModel* const m_sortModel;
    bool m_inEntryListMode;
    /**
     * @author Fonic <https://github.com/fonic>
     * Properties used by header menu
     */
    QMenu* m_headerMenu;
    QAction* m_hideUsernamesAction;
    QAction* m_hidePasswordsAction;
    QList<QAction *> m_columnActions;
    QByteArray m_headerSessionState;
    QByteArray m_headerDefaultState;
};

#endif // KEEPASSX_ENTRYVIEW_H
