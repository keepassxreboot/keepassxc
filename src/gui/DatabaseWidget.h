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

#ifndef KEEPASSX_DATABASEWIDGET_H
#define KEEPASSX_DATABASEWIDGET_H

#include <QtCore/QScopedPointer>
#include <QtGui/QStackedWidget>

class ChangeMasterKeyWidget;
class DatabaseSettingsWidget;
class Database;
class EditEntryWidget;
class EditGroupWidget;
class Entry;
class EntryView;
class Group;
class GroupView;

namespace Ui {
    class SearchWidget;
}

class DatabaseWidget : public QStackedWidget
{
    Q_OBJECT

public:
    enum Mode
    {
        None,
        ViewMode,
        EditMode
    };

    explicit DatabaseWidget(Database* db, QWidget* parent = 0);
    ~DatabaseWidget();
    GroupView* groupView();
    EntryView* entryView();
    bool dbHasKey();
    void cloneEntry();
    void deleteEntry();
    void deleteGroup();

    bool canDeleteCurrentGoup();
    int addWidget(QWidget* w);
    void setCurrentIndex(int index);
    DatabaseWidget::Mode currentMode();

Q_SIGNALS:
    void closeRequest();
    void currentModeChanged(DatabaseWidget::Mode mode);

public Q_SLOTS:
    void createEntry();
    void createGroup();
    void switchToEntryEdit();
    void switchToGroupEdit();
    void switchToMasterKeyChange();
    void switchToDatabaseSettings();
    void toggleSearch();

private Q_SLOTS:
    void switchBackToEntryEdit();
    void switchToView(bool accepted);
    void switchToHistoryView(Entry* entry);
    void switchToEntryEdit(Entry* entry);
    void switchToEntryEdit(Entry* entry, bool create);
    void switchToGroupEdit(Group* entry, bool create);
    void updateMasterKey(bool accepted);
    void emitCurrentModeChanged();
    void clearLastGroup(Group* group);
    void search();
    void startSearchTimer();
    void showSearch();
    void closeSearch();

private:
    void truncateHistories();

    Database* const m_db;
    const QScopedPointer<Ui::SearchWidget> m_searchUi;
    QWidget* const m_searchWidget;
    QWidget* m_mainWidget;
    EditEntryWidget* m_editEntryWidget;
    EditEntryWidget* m_historyEditEntryWidget;
    EditGroupWidget* m_editGroupWidget;
    ChangeMasterKeyWidget* m_changeMasterKeyWidget;
    DatabaseSettingsWidget* m_databaseSettingsWidget;
    GroupView* m_groupView;
    EntryView* m_entryView;
    Group* m_newGroup;
    Entry* m_newEntry;
    Group* m_newParent;
    Group* m_lastGroup;
    QTimer* m_searchTimer;
};

#endif // KEEPASSX_DATABASEWIDGET_H
