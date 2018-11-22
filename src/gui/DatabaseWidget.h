/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include <QFileSystemWatcher>
#include <QScopedPointer>
#include <QStackedWidget>
#include <QTimer>

#include "gui/entry/EntryModel.h"
#include "gui/MessageWidget.h"
#include "gui/csvImport/CsvImportWizard.h"
#include "gui/entry/EntryModel.h"

class DatabaseOpenWidget;
class DatabaseSettingsDialog;
class Database;
class EditEntryWidget;
class EditGroupWidget;
class Entry;
class EntryView;
class EntrySearcher;
class Group;
class GroupView;
class KeePass1OpenWidget;
class QFile;
class QMenu;
class QSplitter;
class QLabel;
class UnlockDatabaseWidget;
class MessageWidget;
class EntryPreviewWidget;
class UnlockDatabaseDialog;
class QFileSystemWatcher;

namespace Ui
{
    class SearchWidget;
}

class DatabaseWidget : public QStackedWidget
{
    Q_OBJECT

public:
    enum class Mode
    {
        None,
        ImportMode,
        ViewMode,
        EditMode,
        LockedMode
    };

    explicit DatabaseWidget(QSharedPointer<Database> db, QWidget* parent = nullptr);
    explicit DatabaseWidget(const QString& filePath, QWidget* parent = nullptr);
    ~DatabaseWidget();

    QSharedPointer<Database> database() const;

    bool lock();
    void prepareUnlock();
    bool save(int attempt = 0);
    bool saveAs();

    DatabaseWidget::Mode currentMode() const;
    bool isLocked() const;
    bool isSearchActive() const;

    QString getCurrentSearch();
    void refreshSearch();

    GroupView* groupView();
    EntryView* entryView();

    Group* currentGroup() const;
    bool canDeleteCurrentGroup() const;
    bool isGroupSelected() const;
    bool isRecycleBinSelected() const;
    int numberOfSelectedEntries() const;

    QStringList customEntryAttributes() const;
    bool isEditWidgetModified() const;
    bool isUsernamesHidden() const;
    void setUsernamesHidden(bool hide);
    bool isPasswordsHidden() const;
    void setPasswordsHidden(bool hide);
    void clearAllWidgets();
    bool currentEntryHasFocus();
    bool currentEntryHasTitle();
    bool currentEntryHasUsername();
    bool currentEntryHasPassword();
    bool currentEntryHasUrl();
    bool currentEntryHasNotes();
    bool currentEntryHasTotp();

    void blockAutoReload(bool block = true);

    QByteArray entryViewState() const;
    bool setEntryViewState(const QByteArray& state) const;
    QList<int> mainSplitterSizes() const;
    void setMainSplitterSizes(const QList<int>& sizes);
    QList<int> previewSplitterSizes() const;
    void setPreviewSplitterSizes(const QList<int>& sizes);

signals:
    // relayed Database signals
    void databaseFilePathChanged(const QString& oldPath, const QString& newPath);
    void databaseModified();
    void databaseSaved();
    void databaseUnlocked();
    void databaseLocked();

    void closeRequest();
    void currentModeChanged(DatabaseWidget::Mode mode);
    void groupChanged();
    void entrySelectionChanged();
    void databaseMerged(QSharedPointer<Database> mergedDb);
    void groupContextMenuRequested(const QPoint& globalPos);
    void entryContextMenuRequested(const QPoint& globalPos);
    void pressedEntry(Entry* selectedEntry);
    void pressedGroup(Group* selectedGroup);
    void listModeAboutToActivate();
    void listModeActivated();
    void searchModeAboutToActivate();
    void searchModeActivated();
    void mainSplitterSizesChanged();
    void previewSplitterSizesChanged();
    void entryViewStateChanged();
    void clearSearch();

public slots:
    void replaceDatabase(QSharedPointer<Database> db);
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
    void showTotp();
    void showTotpKeyQrCode();
    void copyTotp();
    void setupTotp();
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
    void switchToOpenDatabase();
    void switchToOpenDatabase(const QString& filePath);
    void switchToCsvImport(const QString& filePath);
    void csvImportFinished(bool accepted);
    void switchToOpenMergeDatabase(const QString& filePath);
    void switchToOpenMergeDatabase(const QString& filePath, const QString& password, const QString& keyFile);
    void switchToImportKeepass1(const QString& filePath);
    void emptyRecycleBin();

    // Search related slots
    void search(const QString& searchtext);
    void setSearchCaseSensitive(bool state);
    void setSearchLimitGroup(bool state);
    void endSearch();

    void showMessage(const QString& text,
                     MessageWidget::MessageType type,
                     bool showClosebutton = true,
                     int autoHideTimeout = MessageWidget::DefaultAutoHideTimeout);
    void showErrorMessage(const QString& errorMessage);
    void hideMessage();

protected:
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void updateFilePath(const QString& filePath);
    void entryActivationSignalReceived(Entry* entry, EntryModel::ModelColumn column);
    void switchBackToEntryEdit();
    void switchToHistoryView(Entry* entry);
    void switchToEntryEdit(Entry*);
    void switchToEntryEdit(Entry* entry, bool create);
    void switchToGroupEdit(Group* entry, bool create);
    void emitGroupContextMenuRequested(const QPoint& pos);
    void emitEntryContextMenuRequested(const QPoint& pos);
    void emitPressedGroup(Group* currentGroup);
    void emitEntrySelectionChanged();
    void connectDatabaseSignals();
    void loadDatabase(bool accepted);
    void mergeDatabase(bool accepted);
    void unlockDatabase(bool accepted);
    void emitCurrentModeChanged();
    // Database autoreload slots
    void onWatchedFileChanged();
    void reloadDatabaseFile();
    void restoreGroupEntryFocus(const QUuid& groupUuid, const QUuid& EntryUuid);
    void unblockAutoReload();

private:
    int addChildWidget(QWidget* w);
    void setClipboardTextAndMinimize(const QString& text);
    void setIconFromParent();

    QSharedPointer<Database> m_db;

    QPointer<QWidget> m_mainWidget;
    QPointer<QSplitter> m_mainSplitter;
    QPointer<MessageWidget> m_messageWidget;
    QPointer<EntryPreviewWidget> m_previewView;
    QPointer<QSplitter> m_previewSplitter;
    QPointer<QLabel> m_searchingLabel;
    QPointer<CsvImportWizard> m_csvImportWizard;
    QPointer<EditEntryWidget> m_editEntryWidget;
    QPointer<EditGroupWidget> m_editGroupWidget;
    QPointer<EditEntryWidget> m_historyEditEntryWidget;
    QPointer<DatabaseSettingsDialog> m_databaseSettingDialog;
    QPointer<DatabaseOpenWidget> m_databaseOpenWidget;
    QPointer<DatabaseOpenWidget> m_databaseOpenMergeWidget;
    QPointer<KeePass1OpenWidget> m_keepass1OpenWidget;
    QPointer<UnlockDatabaseWidget> m_unlockDatabaseWidget;
    QPointer<UnlockDatabaseDialog> m_unlockDatabaseDialog;
    QPointer<GroupView> m_groupView;
    QPointer<EntryView> m_entryView;

    QPointer<Group> m_newGroup;
    QPointer<Entry> m_newEntry;
    QPointer<Group> m_newParent;

    QUuid m_groupBeforeLock;
    QUuid m_entryBeforeLock;

    QString m_databaseName;
    QString m_databaseFileName;

    // Search state
    EntrySearcher* m_EntrySearcher;
    QString m_lastSearchText;
    bool m_searchLimitGroup;

    // Autoreload
    QFileSystemWatcher m_fileWatcher;
    QTimer m_fileWatchTimer;
    QTimer m_fileWatchUnblockTimer;
    bool m_ignoreAutoReload;
};

#endif // KEEPASSX_DATABASEWIDGET_H
