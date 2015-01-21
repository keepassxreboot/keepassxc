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

#include <QScopedPointer>
#include <QStackedWidget>
#include <QFileSystemWatcher>
#include <QTimer>

#include "core/Uuid.h"

#include "gui/entry/EntryModel.h"
#include "gui/MessageWidget.h"

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
class QSplitter;
class QLabel;
class UnlockDatabaseWidget;
class MessageWidget;
class UnlockDatabaseDialog;
class QFileSystemWatcher;

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
        EditMode,
        LockedMode
    };

    explicit DatabaseWidget(Database* db, QWidget* parent = nullptr);
    ~DatabaseWidget();
    Database* database();
    bool dbHasKey() const;
    bool canDeleteCurrentGroup() const;
    bool isInSearchMode() const;
    QString getCurrentSearch();
    Group* currentGroup() const;
    int addWidget(QWidget* w);
    void setCurrentIndex(int index);
    void setCurrentWidget(QWidget* widget);
    DatabaseWidget::Mode currentMode() const;
    void lock();
    void updateFilename(const QString& filename);
    int numberOfSelectedEntries() const;
    QStringList customEntryAttributes() const;
    bool isGroupSelected() const;
    bool isInEditMode() const;
    bool isEditWidgetModified() const;
    QList<int> splitterSizes() const;
    void setSplitterSizes(const QList<int>& sizes);
    QList<int> entryHeaderViewSizes() const;
    void setEntryViewHeaderSizes(const QList<int>& sizes);
    void clearAllWidgets();
    bool currentEntryHasTitle();
    bool currentEntryHasUsername();
    bool currentEntryHasPassword();
    bool currentEntryHasUrl();
    bool currentEntryHasNotes();
    GroupView* groupView();
    EntryView* entryView();
    void showUnlockDialog();
    void closeUnlockDialog();
    void ignoreNextAutoreload();

Q_SIGNALS:
    void closeRequest();
    void currentModeChanged(DatabaseWidget::Mode mode);
    void groupChanged();
    void entrySelectionChanged();
    void databaseChanged(Database* newDb, bool unsavedChanges);
    void databaseMerged(Database* mergedDb);
    void groupContextMenuRequested(const QPoint& globalPos);
    void entryContextMenuRequested(const QPoint& globalPos);
    void unlockedDatabase();
    void listModeAboutToActivate();
    void listModeActivated();
    void searchModeAboutToActivate();
    void searchModeActivated();
    void splitterSizesChanged();
    void entryColumnSizesChanged();
    void updateSearch(QString text);

public Q_SLOTS:
    void createEntry();
    void cloneEntry();
    void deleteEntries();
    void setFocus();
    void copyTitle();
    void copyUsername();
    void copyPassword();
    void copyURL();
    void copyNotes();
    void copyAttribute(QAction* action);
    void performAutoType();
    void openUrl();
    void openUrlForEntry(Entry* entry);
    void createGroup();
    void deleteGroup();
    void onGroupChanged(Group* group);
    void switchToView(bool accepted);
    void switchToEntryEdit();
    void switchToGroupEdit();
    void switchToMasterKeyChange();
    void switchToDatabaseSettings();
    void switchToOpenDatabase(const QString& fileName);
    void switchToOpenDatabase(const QString& fileName, const QString& password, const QString& keyFile);
    void switchToOpenMergeDatabase(const QString& fileName);
    void switchToOpenMergeDatabase(const QString& fileName, const QString& password, const QString& keyFile);
    void switchToImportKeepass1(const QString& fileName);
    void databaseModified();
    void databaseSaved();
    // Search related slots
    void search(const QString& searchtext);
    void setSearchCaseSensitive(bool state);
    void endSearch();
    void showMessage(const QString& text, MessageWidget::MessageType type);
    void hideMessage();

private Q_SLOTS:
    void entryActivationSignalReceived(Entry* entry, EntryModel::ModelColumn column);
    void switchBackToEntryEdit();
    void switchToHistoryView(Entry* entry);
    void switchToEntryEdit(Entry* entry);
    void switchToEntryEdit(Entry* entry, bool create);
    void switchToGroupEdit(Group* entry, bool create);
    void emitGroupContextMenuRequested(const QPoint& pos);
    void emitEntryContextMenuRequested(const QPoint& pos);
    void updateMasterKey(bool accepted);
    void openDatabase(bool accepted);
    void mergeDatabase(bool accepted);
    void unlockDatabase(bool accepted);
    void emitCurrentModeChanged();
    // Database autoreload slots
    void onWatchedFileChanged();
    void reloadDatabaseFile();
    void restoreGroupEntryFocus(Uuid groupUuid, Uuid EntryUuid);

private:
    void setClipboardTextAndMinimize(const QString& text);
    void setIconFromParent();
    void replaceDatabase(Database* db);

    Database* m_db;
    QWidget* m_mainWidget;
    EditEntryWidget* m_editEntryWidget;
    EditEntryWidget* m_historyEditEntryWidget;
    EditGroupWidget* m_editGroupWidget;
    ChangeMasterKeyWidget* m_changeMasterKeyWidget;
    DatabaseSettingsWidget* m_databaseSettingsWidget;
    DatabaseOpenWidget* m_databaseOpenWidget;
    DatabaseOpenWidget* m_databaseOpenMergeWidget;
    KeePass1OpenWidget* m_keepass1OpenWidget;
    UnlockDatabaseWidget* m_unlockDatabaseWidget;
    UnlockDatabaseDialog* m_unlockDatabaseDialog;
    QSplitter* m_splitter;
    GroupView* m_groupView;
    EntryView* m_entryView;
    QLabel* m_searchingLabel;
    Group* m_newGroup;
    Entry* m_newEntry;
    Group* m_newParent;
    QString m_filename;
    Uuid m_groupBeforeLock;
    Uuid m_entryBeforeLock;
    MessageWidget* m_messageWidget;

    // Search state
    QString m_lastSearchText;
    bool m_searchCaseSensitive;

    // Autoreload
    QFileSystemWatcher m_fileWatcher;
    QTimer m_fileWatchTimer;
    bool m_ignoreNextAutoreload;
    QTimer m_ignoreWatchTimer;
    bool m_databaseModified;
};

#endif // KEEPASSX_DATABASEWIDGET_H
