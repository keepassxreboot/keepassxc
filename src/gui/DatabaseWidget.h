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

#include "core/Global.h"

class ChangeMasterKeyWidget;
class DatabaseOpenWidget;
class DatabaseSettingsWidget;
class Database;
class EditEntryWidget;
class EditGroupWidget;
class Entry;
class EntryView;
class Group;
class GroupView;
class KeePass1OpenWidget;
class QFile;
class QMenu;

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

    explicit DatabaseWidget(Database* db, QWidget* parent = Q_NULLPTR);
    ~DatabaseWidget();
    GroupView* groupView();
    EntryView* entryView();
    Database* database();
    bool dbHasKey();
    bool canDeleteCurrentGoup();
    bool isInSearchMode();
    int addWidget(QWidget* w);
    void setCurrentIndex(int index);
    DatabaseWidget::Mode currentMode();

Q_SIGNALS:
    void closeRequest();
    void currentModeChanged(DatabaseWidget::Mode mode);
    void groupChanged();
    void entrySelectionChanged();
    void databaseChanged(Database* newDb);
    void groupContextMenuRequested(const QPoint& globalPos);
    void entryContextMenuRequested(const QPoint& globalPos);

public Q_SLOTS:
    void createEntry();
    void cloneEntry();
    void deleteEntry();
    void copyUsername();
    void copyPassword();
    void performAutoType();
    void openUrl();
    void createGroup();
    void deleteGroup();
    void switchToEntryEdit();
    void switchToGroupEdit();
    void switchToMasterKeyChange();
    void switchToDatabaseSettings();
    void switchToOpenDatabase(const QString& fileName);
    void switchToOpenDatabase(const QString& fileName, const QString& password, const QString& keyFile);
    void switchToImportKeepass1(const QString& fileName);
    void toggleSearch();
    void emitGroupContextMenuRequested(const QPoint& pos);
    void emitEntryContextMenuRequested(const QPoint& pos);

private Q_SLOTS:
    void switchBackToEntryEdit();
    void switchToView(bool accepted);
    void switchToHistoryView(Entry* entry);
    void switchToEntryEdit(Entry* entry);
    void switchToEntryEdit(Entry* entry, bool create);
    void switchToGroupEdit(Group* entry, bool create);
    void updateMasterKey(bool accepted);
    void openDatabase(bool accepted);
    void emitCurrentModeChanged();
    void clearLastGroup(Group* group);
    void search();
    void startSearch();
    void startSearchTimer();
    void showSearch();
    void closeSearch();

private:
    Database* m_db;
    const QScopedPointer<Ui::SearchWidget> m_searchUi;
    QWidget* const m_searchWidget;
    QWidget* m_mainWidget;
    EditEntryWidget* m_editEntryWidget;
    EditEntryWidget* m_historyEditEntryWidget;
    EditGroupWidget* m_editGroupWidget;
    ChangeMasterKeyWidget* m_changeMasterKeyWidget;
    DatabaseSettingsWidget* m_databaseSettingsWidget;
    DatabaseOpenWidget* m_databaseOpenWidget;
    KeePass1OpenWidget* m_keepass1OpenWidget;
    GroupView* m_groupView;
    EntryView* m_entryView;
    Group* m_newGroup;
    Entry* m_newEntry;
    Group* m_newParent;
    Group* m_lastGroup;
    QTimer* m_searchTimer;
};

#endif // KEEPASSX_DATABASEWIDGET_H
