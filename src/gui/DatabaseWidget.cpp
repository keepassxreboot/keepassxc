/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "DatabaseWidget.h"

#include <QAction>
#include <QDesktopServices>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QFile>
#include <QLineEdit>
#include <QKeyEvent>
#include <QSplitter>
#include <QLabel>
#include <QProcess>
#include <QHeaderView>
#include <QApplication>

#include "autotype/AutoType.h"
#include "core/Config.h"
#include "core/EntrySearcher.h"
#include "core/FilePath.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "format/KeePass2Reader.h"
#include "gui/ChangeMasterKeyWidget.h"
#include "gui/Clipboard.h"
#include "gui/CloneDialog.h"
#include "gui/SetupTotpDialog.h"
#include "gui/TotpDialog.h"
#include "gui/DatabaseOpenWidget.h"
#include "gui/DatabaseSettingsWidget.h"
#include "gui/KeePass1OpenWidget.h"
#include "gui/MessageBox.h"
#include "gui/UnlockDatabaseWidget.h"
#include "gui/UnlockDatabaseDialog.h"
#include "gui/entry/EditEntryWidget.h"
#include "gui/entry/EntryView.h"
#include "gui/group/EditGroupWidget.h"
#include "gui/group/GroupView.h"

DatabaseWidget::DatabaseWidget(Database* db, QWidget* parent)
    : QStackedWidget(parent)
    , m_db(db)
    , m_newGroup(nullptr)
    , m_newEntry(nullptr)
    , m_newParent(nullptr)
{
    m_mainWidget = new QWidget(this);

    m_messageWidget = new MessageWidget(this);
    m_messageWidget->setHidden(true);

    QVBoxLayout* mainLayout = new QVBoxLayout();
    QLayout* layout = new QHBoxLayout();
    mainLayout->addWidget(m_messageWidget);
    mainLayout->addLayout(layout);
    m_splitter = new QSplitter(m_mainWidget);
    m_splitter->setChildrenCollapsible(false);

    QWidget* rightHandSideWidget = new QWidget(m_splitter);

    m_groupView = new GroupView(db, m_splitter);
    m_groupView->setObjectName("groupView");
    m_groupView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_groupView, SIGNAL(customContextMenuRequested(QPoint)),
            SLOT(emitGroupContextMenuRequested(QPoint)));

    m_entryView = new EntryView(rightHandSideWidget);
    m_entryView->setObjectName("entryView");
    m_entryView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_entryView->setGroup(db->rootGroup());
    connect(m_entryView, SIGNAL(customContextMenuRequested(QPoint)),
            SLOT(emitEntryContextMenuRequested(QPoint)));

    // Add a notification for when we are searching
    m_searchingLabel = new QLabel();
    m_searchingLabel->setText(tr("Searching..."));
    m_searchingLabel->setAlignment(Qt::AlignCenter);
    m_searchingLabel->setStyleSheet("color: rgb(0, 0, 0);"
                                    "background-color: rgb(255, 253, 160);"
                                    "border: 2px solid rgb(190, 190, 190);"
                                    "border-radius: 5px;");

    QVBoxLayout* vLayout = new QVBoxLayout(rightHandSideWidget);
    vLayout->setMargin(0);
    vLayout->addWidget(m_searchingLabel);
    vLayout->addWidget(m_entryView);

    m_searchingLabel->setVisible(false);

    rightHandSideWidget->setLayout(vLayout);

    setTabOrder(m_entryView, m_groupView);

    m_splitter->addWidget(m_groupView);
    m_splitter->addWidget(rightHandSideWidget);

    m_splitter->setStretchFactor(0, 30);
    m_splitter->setStretchFactor(1, 70);

    layout->addWidget(m_splitter);
    m_mainWidget->setLayout(mainLayout);

    m_editEntryWidget = new EditEntryWidget();
    m_editEntryWidget->setObjectName("editEntryWidget");
    m_historyEditEntryWidget = new EditEntryWidget();
    m_editGroupWidget = new EditGroupWidget();
    m_editGroupWidget->setObjectName("editGroupWidget");
    m_changeMasterKeyWidget = new ChangeMasterKeyWidget();
    m_changeMasterKeyWidget->headlineLabel()->setText(tr("Change master key"));
    m_csvImportWizard = new CsvImportWizard();
    m_csvImportWizard->setObjectName("csvImportWizard");
    QFont headlineLabelFont = m_changeMasterKeyWidget->headlineLabel()->font();
    headlineLabelFont.setBold(true);
    headlineLabelFont.setPointSize(headlineLabelFont.pointSize() + 2);
    m_changeMasterKeyWidget->headlineLabel()->setFont(headlineLabelFont);
    m_databaseSettingsWidget = new DatabaseSettingsWidget();
    m_databaseSettingsWidget->setObjectName("databaseSettingsWidget");
    m_databaseOpenWidget = new DatabaseOpenWidget();
    m_databaseOpenWidget->setObjectName("databaseOpenWidget");
    m_databaseOpenMergeWidget = new DatabaseOpenWidget();
    m_databaseOpenMergeWidget->setObjectName("databaseOpenMergeWidget");
    m_keepass1OpenWidget = new KeePass1OpenWidget();
    m_keepass1OpenWidget->setObjectName("keepass1OpenWidget");
    m_unlockDatabaseWidget = new UnlockDatabaseWidget();
    m_unlockDatabaseWidget->setObjectName("unlockDatabaseWidget");
    m_unlockDatabaseDialog = new UnlockDatabaseDialog();
    m_unlockDatabaseDialog->setObjectName("unlockDatabaseDialog");
    addWidget(m_mainWidget);
    addWidget(m_editEntryWidget);
    addWidget(m_editGroupWidget);
    addWidget(m_changeMasterKeyWidget);
    addWidget(m_databaseSettingsWidget);
    addWidget(m_historyEditEntryWidget);
    addWidget(m_databaseOpenWidget);
    addWidget(m_csvImportWizard);
    addWidget(m_databaseOpenMergeWidget);
    addWidget(m_keepass1OpenWidget);
    addWidget(m_unlockDatabaseWidget);

    connect(m_splitter, SIGNAL(splitterMoved(int,int)), SIGNAL(splitterSizesChanged()));
    connect(m_entryView->header(), SIGNAL(sectionResized(int,int,int)), SIGNAL(entryColumnSizesChanged()));
    connect(m_groupView, SIGNAL(groupChanged(Group*)), this, SLOT(onGroupChanged(Group*)));
    connect(m_groupView, SIGNAL(groupChanged(Group*)), SIGNAL(groupChanged()));
    connect(m_entryView, SIGNAL(entryActivated(Entry*, EntryModel::ModelColumn)),
            SLOT(entryActivationSignalReceived(Entry*, EntryModel::ModelColumn)));
    connect(m_entryView, SIGNAL(entrySelectionChanged()), SIGNAL(entrySelectionChanged()));
    connect(m_editEntryWidget, SIGNAL(editFinished(bool)), SLOT(switchToView(bool)));
    connect(m_editEntryWidget, SIGNAL(historyEntryActivated(Entry*)), SLOT(switchToHistoryView(Entry*)));
    connect(m_historyEditEntryWidget, SIGNAL(editFinished(bool)), SLOT(switchBackToEntryEdit()));
    connect(m_editGroupWidget, SIGNAL(editFinished(bool)), SLOT(switchToView(bool)));
    connect(m_changeMasterKeyWidget, SIGNAL(editFinished(bool)), SLOT(updateMasterKey(bool)));
    connect(m_databaseSettingsWidget, SIGNAL(editFinished(bool)), SLOT(switchToView(bool)));
    connect(m_databaseOpenWidget, SIGNAL(editFinished(bool)), SLOT(openDatabase(bool)));
    connect(m_databaseOpenMergeWidget, SIGNAL(editFinished(bool)), SLOT(mergeDatabase(bool)));
    connect(m_keepass1OpenWidget, SIGNAL(editFinished(bool)), SLOT(openDatabase(bool)));
    connect(m_csvImportWizard, SIGNAL(importFinished(bool)), SLOT(csvImportFinished(bool)));
    connect(m_unlockDatabaseWidget, SIGNAL(editFinished(bool)), SLOT(unlockDatabase(bool)));
	connect(m_unlockDatabaseDialog, SIGNAL(unlockDone(bool)), SLOT(unlockDatabase(bool)));
    connect(&m_fileWatcher, SIGNAL(fileChanged(QString)), this, SLOT(onWatchedFileChanged()));
    connect(&m_fileWatchTimer, SIGNAL(timeout()), this, SLOT(reloadDatabaseFile()));
    connect(&m_fileWatchUnblockTimer, SIGNAL(timeout()), this, SLOT(unblockAutoReload()));
    connect(this, SIGNAL(currentChanged(int)), this, SLOT(emitCurrentModeChanged()));

    m_databaseModified = false;

    m_fileWatchTimer.setSingleShot(true);
    m_fileWatchUnblockTimer.setSingleShot(true);
    m_ignoreAutoReload = false;

    m_searchCaseSensitive = false;
    m_searchLimitGroup = config()->get("SearchLimitGroup", false).toBool();

    setCurrentWidget(m_mainWidget);
}

DatabaseWidget::~DatabaseWidget()
{
}

DatabaseWidget::Mode DatabaseWidget::currentMode() const
{
    if (currentWidget() == nullptr) {
        return DatabaseWidget::None;
    }
    else if (currentWidget() == m_csvImportWizard) {
        return DatabaseWidget::ImportMode;
    }
    else if (currentWidget() == m_mainWidget) {
        return DatabaseWidget::ViewMode;
    }
    else if (currentWidget() == m_unlockDatabaseWidget ||
             currentWidget() == m_databaseOpenWidget) {
        return DatabaseWidget::LockedMode;
    }
    else {
        return DatabaseWidget::EditMode;
    }
}

bool DatabaseWidget::isInEditMode() const
{
    return currentMode() == DatabaseWidget::EditMode;
}

bool DatabaseWidget::isEditWidgetModified() const
{
    if (currentWidget() == m_editEntryWidget) {
        return m_editEntryWidget->hasBeenModified();
    }
    else {
        // other edit widget don't have a hasBeenModified() method yet
        // assume that they already have been modified
        return true;
    }
}

QList<int> DatabaseWidget::splitterSizes() const
{
    return m_splitter->sizes();
}

void DatabaseWidget::setSplitterSizes(const QList<int>& sizes)
{
    m_splitter->setSizes(sizes);
}

QList<int> DatabaseWidget::entryHeaderViewSizes() const
{
    QList<int> sizes;

    for (int i = 0; i < m_entryView->header()->count(); i++) {
        sizes.append(m_entryView->header()->sectionSize(i));
    }

    return sizes;
}

void DatabaseWidget::setEntryViewHeaderSizes(const QList<int>& sizes)
{
    if (sizes.size() != m_entryView->header()->count()) {
        Q_ASSERT(false);
        return;
    }

    for (int i = 0; i < sizes.size(); i++) {
        m_entryView->header()->resizeSection(i, sizes[i]);
    }
}

void DatabaseWidget::clearAllWidgets()
{
    m_editEntryWidget->clear();
    m_historyEditEntryWidget->clear();
    m_editGroupWidget->clear();
}

void DatabaseWidget::emitCurrentModeChanged()
{
    emit currentModeChanged(currentMode());
}

Database* DatabaseWidget::database()
{
    return m_db;
}

void DatabaseWidget::createEntry()
{
    if (!m_groupView->currentGroup()) {
        Q_ASSERT(false);
        return;
    }

    m_newEntry = new Entry();
    m_newEntry->setUuid(Uuid::random());
    m_newEntry->setUsername(m_db->metadata()->defaultUserName());
    m_newParent = m_groupView->currentGroup();
    setIconFromParent();
    switchToEntryEdit(m_newEntry, true);
}

void DatabaseWidget::setIconFromParent()
{
    if (!config()->get("UseGroupIconOnEntryCreation").toBool()) {
        return;
    }

    if (m_newParent->iconNumber() == Group::DefaultIconNumber && m_newParent->iconUuid().isNull()) {
        return;
    }

    if (m_newParent->iconUuid().isNull()) {
        m_newEntry->setIcon(m_newParent->iconNumber());
    }
    else {
        m_newEntry->setIcon(m_newParent->iconUuid());
    }
}

void DatabaseWidget::replaceDatabase(Database* db)
{
    Database* oldDb = m_db;
    m_db = db;
    m_groupView->changeDatabase(m_db);
    emit databaseChanged(m_db, m_databaseModified);
    delete oldDb;
}

void DatabaseWidget::cloneEntry()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return;
    }

    CloneDialog* cloneDialog = new CloneDialog(this, m_db, currentEntry);
    cloneDialog->show();
    return;
}

void DatabaseWidget::showTotp()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return;
    }

    TotpDialog* totpDialog = new TotpDialog(this, currentEntry);
    totpDialog->open();
}

void DatabaseWidget::copyTotp()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return;
    }
    setClipboardTextAndMinimize(currentEntry->totp());
}

void DatabaseWidget::setupTotp()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return;
    }

    SetupTotpDialog* setupTotpDialog = new SetupTotpDialog(this, currentEntry);
    if (currentEntry->hasTotp()) {
        setupTotpDialog->setSeed(currentEntry->totpSeed());
        setupTotpDialog->setStep(currentEntry->totpStep());
        setupTotpDialog->setDigits(currentEntry->totpDigits());
    }

    setupTotpDialog->open();

}


void DatabaseWidget::deleteEntries()
{
    const QModelIndexList selected = m_entryView->selectionModel()->selectedRows();

    if (selected.isEmpty()) {
        Q_ASSERT(false);
        return;
    }

    // get all entry pointers as the indexes change when removing multiple entries
    QList<Entry*> selectedEntries;
    for (const QModelIndex& index : selected) {
        selectedEntries.append(m_entryView->entryFromIndex(index));
    }

    bool inRecycleBin = Tools::hasChild(m_db->metadata()->recycleBin(), selectedEntries.first());
    if (inRecycleBin || !m_db->metadata()->recycleBinEnabled()) {
        QMessageBox::StandardButton result;

        if (selected.size() == 1) {
            result = MessageBox::question(
                this, tr("Delete entry?"),
                tr("Do you really want to delete the entry \"%1\" for good?")
                .arg(selectedEntries.first()->title().toHtmlEscaped()),
                QMessageBox::Yes | QMessageBox::No);
        }
        else {
            result = MessageBox::question(
                this, tr("Delete entries?"),
                tr("Do you really want to delete %1 entries for good?")
                .arg(selected.size()),
                QMessageBox::Yes | QMessageBox::No);
        }

        if (result == QMessageBox::Yes) {
            for (Entry* entry : asConst(selectedEntries)) {
                delete entry;
            }
            refreshSearch();
        }
    }
    else {
        QMessageBox::StandardButton result;

        if (selected.size() == 1) {
            result = MessageBox::question(
                this, tr("Move entry to recycle bin?"),
                tr("Do you really want to move entry \"%1\" to the recycle bin?")
                .arg(selectedEntries.first()->title().toHtmlEscaped()),
                QMessageBox::Yes | QMessageBox::No);
        }
        else {
            result = MessageBox::question(
                this, tr("Move entries to recycle bin?"),
                tr("Do you really want to move %n entry(s) to the recycle bin?", 0, selected.size()),
                QMessageBox::Yes | QMessageBox::No);
        }

        if (result == QMessageBox::No) {
            return;
        }

        for (Entry* entry : asConst(selectedEntries)) {
            m_db->recycleEntry(entry);
        }
    }
}

void DatabaseWidget::setFocus()
{
	m_entryView->setFocus();
}

void DatabaseWidget::copyTitle()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return;
    }

    setClipboardTextAndMinimize(currentEntry->resolveMultiplePlaceholders(currentEntry->title()));
}

void DatabaseWidget::copyUsername()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return;
    }

    setClipboardTextAndMinimize(currentEntry->resolveMultiplePlaceholders(currentEntry->username()));
}

void DatabaseWidget::copyPassword()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return;
    }

    setClipboardTextAndMinimize(currentEntry->resolveMultiplePlaceholders(currentEntry->password()));
}

void DatabaseWidget::copyURL()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return;
    }

    setClipboardTextAndMinimize(currentEntry->resolveMultiplePlaceholders(currentEntry->url()));
}

void DatabaseWidget::copyNotes()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return;
    }

    setClipboardTextAndMinimize(currentEntry->resolveMultiplePlaceholders(currentEntry->notes()));
}

void DatabaseWidget::copyAttribute(QAction* action)
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return;
    }

    setClipboardTextAndMinimize(currentEntry->resolveMultiplePlaceholders(currentEntry->attributes()->value(action->text())));
}

void DatabaseWidget::setClipboardTextAndMinimize(const QString& text)
{
    clipboard()->setText(text);
    if (config()->get("MinimizeOnCopy").toBool()) {
        window()->showMinimized();
    }
}

void DatabaseWidget::performAutoType()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return;
    }

    autoType()->performAutoType(currentEntry, window());
}

void DatabaseWidget::openUrl()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return;
    }

    openUrlForEntry(currentEntry);
}

void DatabaseWidget::openUrlForEntry(Entry* entry)
{
    QString urlString = entry->resolveMultiplePlaceholders(entry->url());
    if (urlString.isEmpty()) {
        return;
    }

    if (urlString.startsWith("cmd://")) {
        // check if decision to execute command was stored
        if (entry->attributes()->hasKey(EntryAttributes::RememberCmdExecAttr)) {
            if (entry->attributes()->value(EntryAttributes::RememberCmdExecAttr) == "1") {
                QProcess::startDetached(urlString.mid(6));
            }
            return;
        }
        
        // otherwise ask user
        if (urlString.length() > 6) {
            QString cmdTruncated = urlString.mid(6);
            if (cmdTruncated.length() > 400)
                cmdTruncated = cmdTruncated.left(400) + " […]";
            QMessageBox msgbox(QMessageBox::Icon::Question,
                               tr("Execute command?"),
                               tr("Do you really want to execute the following command?<br><br>%1<br>")
                                   .arg(cmdTruncated.toHtmlEscaped()),
                               QMessageBox::Yes | QMessageBox::No,
                               this
            );
            msgbox.setDefaultButton(QMessageBox::No);
            
            QCheckBox* checkbox = new QCheckBox(tr("Remember my choice"), &msgbox);
            msgbox.setCheckBox(checkbox);
            bool remember = false;
            QObject::connect(checkbox, &QCheckBox::stateChanged, [&](int state) {
                if (static_cast<Qt::CheckState>(state) == Qt::CheckState::Checked) {
                   remember = true;
               }
            });
            
            int result = msgbox.exec();
            if (result == QMessageBox::Yes) {
                QProcess::startDetached(urlString.mid(6));
            }
            
            if (remember) {
                entry->attributes()->set(EntryAttributes::RememberCmdExecAttr,
                                         result == QMessageBox::Yes ? "1" : "0");
            }
        }
    }
    else {
        QUrl url = QUrl::fromUserInput(urlString);
        QDesktopServices::openUrl(url);
    }
}

void DatabaseWidget::createGroup()
{
    if (!m_groupView->currentGroup()) {
        Q_ASSERT(false);
        return;
    }

    m_newGroup = new Group();
    m_newGroup->setUuid(Uuid::random());
    m_newParent = m_groupView->currentGroup();
    switchToGroupEdit(m_newGroup, true);
}

void DatabaseWidget::deleteGroup()
{
    Group* currentGroup = m_groupView->currentGroup();
    if (!currentGroup || !canDeleteCurrentGroup()) {
        Q_ASSERT(false);
        return;
    }

    bool inRecycleBin = Tools::hasChild(m_db->metadata()->recycleBin(), currentGroup);
    bool isRecycleBin = (currentGroup == m_db->metadata()->recycleBin());
    bool isRecycleBinSubgroup = Tools::hasChild(currentGroup, m_db->metadata()->recycleBin());
    if (inRecycleBin || isRecycleBin || isRecycleBinSubgroup || !m_db->metadata()->recycleBinEnabled()) {
        QMessageBox::StandardButton result = MessageBox::question(
            this, tr("Delete group?"),
            tr("Do you really want to delete the group \"%1\" for good?")
            .arg(currentGroup->name().toHtmlEscaped()),
            QMessageBox::Yes | QMessageBox::No);
        if (result == QMessageBox::Yes) {
            delete currentGroup;
        }
    }
    else {
        m_db->recycleGroup(currentGroup);
    }
}

int DatabaseWidget::addWidget(QWidget* w)
{
    w->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    int index = QStackedWidget::addWidget(w);

    adjustSize();

    return index;
}

void DatabaseWidget::setCurrentIndex(int index)
{
    // use setCurrentWidget() instead
    // index is not reliable
    Q_UNUSED(index);
    Q_ASSERT(false);
}

void DatabaseWidget::setCurrentWidget(QWidget* widget)
{
    if (currentWidget()) {
        currentWidget()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    }

    QStackedWidget::setCurrentWidget(widget);

    if (currentWidget()) {
        currentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }

    adjustSize();
}

void DatabaseWidget::csvImportFinished(bool accepted)
{
    if (!accepted) {
        emit closeRequest();
    }
    else {
        setCurrentWidget(m_mainWidget);
    }
}

void DatabaseWidget::switchToView(bool accepted)
{
    if (m_newGroup) {
        if (accepted) {
            m_newGroup->setParent(m_newParent);
            m_groupView->setCurrentGroup(m_newGroup);
            m_groupView->expandGroup(m_newParent);
        }
        else {
            delete m_newGroup;
        }

        m_newGroup = nullptr;
        m_newParent = nullptr;
    }
    else if (m_newEntry) {
        if (accepted) {
            m_newEntry->setGroup(m_newParent);
            m_entryView->setFocus();
            m_entryView->setCurrentEntry(m_newEntry);
        }
        else {
            delete m_newEntry;
        }

        m_newEntry = nullptr;
        m_newParent = nullptr;
    }

    setCurrentWidget(m_mainWidget);
}

void DatabaseWidget::switchToHistoryView(Entry* entry)
{
    m_historyEditEntryWidget->loadEntry(entry, false, true, m_editEntryWidget->entryTitle(), m_db);
    setCurrentWidget(m_historyEditEntryWidget);
}

void DatabaseWidget::switchBackToEntryEdit()
{
    setCurrentWidget(m_editEntryWidget);
}

void DatabaseWidget::switchToEntryEdit(Entry* entry)
{
    switchToEntryEdit(entry, false);
}

void DatabaseWidget::switchToEntryEdit(Entry* entry, bool create)
{
    Group* group = currentGroup();
    Q_ASSERT(group);

    m_editEntryWidget->loadEntry(entry, create, false, group->name(), m_db);
    setCurrentWidget(m_editEntryWidget);
}

void DatabaseWidget::switchToGroupEdit(Group* group, bool create)
{
    m_editGroupWidget->loadGroup(group, create, m_db);
    setCurrentWidget(m_editGroupWidget);
}

void DatabaseWidget::updateMasterKey(bool accepted)
{
    if (accepted) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        bool result = m_db->setKey(m_changeMasterKeyWidget->newMasterKey());
        QApplication::restoreOverrideCursor();

        if (!result) {
            m_messageWidget->showMessage(tr("Unable to calculate master key"), MessageWidget::Error);
            return;
        }
    }
    else if (!m_db->hasKey()) {
        emit closeRequest();
        return;
    }

    setCurrentWidget(m_mainWidget);
}

void DatabaseWidget::openDatabase(bool accepted)
{
    if (accepted) {
        replaceDatabase(static_cast<DatabaseOpenWidget*>(sender())->database());
        setCurrentWidget(m_mainWidget);
        emit unlockedDatabase();

        // We won't need those anymore and KeePass1OpenWidget closes
        // the file in its dtor.
        delete m_databaseOpenWidget;
        m_databaseOpenWidget = nullptr;
        delete m_keepass1OpenWidget;
        m_keepass1OpenWidget = nullptr;
        m_fileWatcher.addPath(m_filename);
    }
    else {
        m_fileWatcher.removePath(m_filename);
        if (m_databaseOpenWidget->database()) {
            delete m_databaseOpenWidget->database();
        }
        emit closeRequest();
    }
}

void DatabaseWidget::mergeDatabase(bool accepted)
{
    if (accepted) {
        if (!m_db) {
            m_messageWidget->showMessage(tr("No current database."), MessageWidget::Error);
            return;
        }

        Database* srcDb = static_cast<DatabaseOpenWidget*>(sender())->database();

        if (!srcDb) {
            m_messageWidget->showMessage(tr("No source database, nothing to do."), MessageWidget::Error);
            return;
        }

        m_db->merge(srcDb);
    }

    m_databaseOpenMergeWidget->clearForms();
    setCurrentWidget(m_mainWidget);
    emit databaseMerged(m_db);
}

void DatabaseWidget::unlockDatabase(bool accepted)
{
    if (!accepted) {
        emit closeRequest();
        return;
    }

    Database *db = Q_NULLPTR;
    if (sender() == m_unlockDatabaseDialog) {
        db = m_unlockDatabaseDialog->database();
    } else if (sender() == m_unlockDatabaseWidget) {
        db = m_unlockDatabaseWidget->database();
    }

    replaceDatabase(db);

    restoreGroupEntryFocus(m_groupBeforeLock, m_entryBeforeLock);
    m_groupBeforeLock = Uuid();
    m_entryBeforeLock = Uuid();

    setCurrentWidget(m_mainWidget);
    m_unlockDatabaseWidget->clearForms();
    emit unlockedDatabase();

    if (sender() == m_unlockDatabaseDialog) {
        QList<Database*> dbList;
        dbList.append(m_db);
        autoType()->performGlobalAutoType(dbList);
    }
}

void DatabaseWidget::entryActivationSignalReceived(Entry* entry, EntryModel::ModelColumn column)
{
    if (column == EntryModel::Url && !entry->url().isEmpty()) {
        openUrlForEntry(entry);
    }
    else {
        switchToEntryEdit(entry);
    }
}

void DatabaseWidget::switchToEntryEdit()
{
    Entry* entry = m_entryView->currentEntry();
    
    if (!entry) {
        return;
    }

    switchToEntryEdit(entry, false);
}

void DatabaseWidget::switchToGroupEdit()
{
    Group* group = m_groupView->currentGroup();
    
    if (!group) {
        return;
    }

    switchToGroupEdit(group, false);
}

void DatabaseWidget::switchToMasterKeyChange(bool disableCancel)
{
    m_changeMasterKeyWidget->clearForms();
    m_changeMasterKeyWidget->setCancelEnabled(!disableCancel);
    setCurrentWidget(m_changeMasterKeyWidget);
}

void DatabaseWidget::switchToDatabaseSettings()
{
    m_databaseSettingsWidget->load(m_db);
    setCurrentWidget(m_databaseSettingsWidget);
}

void DatabaseWidget::switchToOpenDatabase(const QString& fileName)
{
    updateFilename(fileName);
    m_databaseOpenWidget->load(fileName);
    setCurrentWidget(m_databaseOpenWidget);
}

void DatabaseWidget::switchToOpenDatabase(const QString& fileName, const QString& password,
                                          const QString& keyFile)
{
    updateFilename(fileName);
    switchToOpenDatabase(fileName);
    m_databaseOpenWidget->enterKey(password, keyFile);
}

void DatabaseWidget::switchToImportCsv(const QString& fileName)
{
    updateFilename(fileName);
    switchToMasterKeyChange();
    m_csvImportWizard->load(fileName, m_db);
    setCurrentWidget(m_csvImportWizard);
}

void DatabaseWidget::switchToOpenMergeDatabase(const QString& fileName)
{
    m_databaseOpenMergeWidget->clearForms();
    m_databaseOpenMergeWidget->load(fileName);
    setCurrentWidget(m_databaseOpenMergeWidget);
}


void DatabaseWidget::switchToOpenMergeDatabase(const QString& fileName, const QString& password,
                                          const QString& keyFile)
{
    switchToOpenMergeDatabase(fileName);
    m_databaseOpenMergeWidget->enterKey(password, keyFile);
}

void DatabaseWidget::switchToImportKeepass1(const QString& fileName)
{
    updateFilename(fileName);
    m_keepass1OpenWidget->load(fileName);
    setCurrentWidget(m_keepass1OpenWidget);
}

void DatabaseWidget::databaseModified()
{
    m_databaseModified = true;
}

void DatabaseWidget::databaseSaved()
{
    m_databaseModified = false;
}

void DatabaseWidget::refreshSearch() {
    if (isInSearchMode()) {
        search(m_lastSearchText);
    }
}

void DatabaseWidget::search(const QString& searchtext)
{
    if (searchtext.isEmpty())
    {
        endSearch();
        return;
    }

    emit searchModeAboutToActivate();

    Qt::CaseSensitivity caseSensitive = m_searchCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    Group* searchGroup = m_searchLimitGroup ? currentGroup() : m_db->rootGroup();

    QList<Entry*> searchResult = EntrySearcher().search(searchtext, searchGroup, caseSensitive);

    m_entryView->setEntryList(searchResult);
    m_lastSearchText = searchtext;

    // Display a label detailing our search results
    if (searchResult.size() > 0) {
        m_searchingLabel->setText(tr("Search Results (%1)").arg(searchResult.size()));
    }
    else {
        m_searchingLabel->setText(tr("No Results"));
    }

    m_searchingLabel->setVisible(true);

    emit searchModeActivated();
}

void DatabaseWidget::setSearchCaseSensitive(bool state)
{
    m_searchCaseSensitive = state;
    refreshSearch();
}

void DatabaseWidget::setSearchLimitGroup(bool state)
{
    m_searchLimitGroup = state;
    refreshSearch();
}

void DatabaseWidget::onGroupChanged(Group* group)
{
    // Intercept group changes if in search mode
    if (isInSearchMode())
        search(m_lastSearchText);
    else
        m_entryView->setGroup(group);
}

QString DatabaseWidget::getCurrentSearch()
{
    return m_lastSearchText;
}

void DatabaseWidget::endSearch()
{
    if (isInSearchMode())
    {
        emit listModeAboutToActivate();

        // Show the normal entry view of the current group
        m_entryView->setGroup(currentGroup());

        emit listModeActivated();
    }

    m_searchingLabel->setVisible(false);
    m_searchingLabel->setText(tr("Searching..."));

    m_lastSearchText.clear();
}

void DatabaseWidget::emitGroupContextMenuRequested(const QPoint& pos)
{
    emit groupContextMenuRequested(m_groupView->viewport()->mapToGlobal(pos));
}

void DatabaseWidget::emitEntryContextMenuRequested(const QPoint& pos)
{
    emit entryContextMenuRequested(m_entryView->viewport()->mapToGlobal(pos));
}

bool DatabaseWidget::dbHasKey() const
{
    return m_db->hasKey();
}

bool DatabaseWidget::canDeleteCurrentGroup() const
{
    bool isRootGroup = m_db->rootGroup() == m_groupView->currentGroup();
    return !isRootGroup;
}

bool DatabaseWidget::isInSearchMode() const
{
    return m_entryView->inEntryListMode();
}

Group* DatabaseWidget::currentGroup() const
{
    return m_groupView->currentGroup();
}

void DatabaseWidget::lock()
{
    Q_ASSERT(currentMode() != DatabaseWidget::LockedMode);

    if (m_groupView->currentGroup()) {
        m_groupBeforeLock = m_groupView->currentGroup()->uuid();
    }
    else {
        m_groupBeforeLock = m_db->rootGroup()->uuid();
    }

    if (m_entryView->currentEntry()) {
        m_entryBeforeLock = m_entryView->currentEntry()->uuid();
    }

    endSearch();
    clearAllWidgets();
    m_unlockDatabaseWidget->load(m_filename);
    setCurrentWidget(m_unlockDatabaseWidget);
    Database* newDb = new Database();
    newDb->metadata()->setName(m_db->metadata()->name());
    replaceDatabase(newDb);
}

void DatabaseWidget::updateFilename(const QString& fileName)
{
    if (!m_filename.isEmpty()) {
        m_fileWatcher.removePath(m_filename);
    }

    m_fileWatcher.addPath(fileName);
    m_filename = fileName;
}

void DatabaseWidget::blockAutoReload(bool block)
{
    if (block) {
        m_ignoreAutoReload = true;
        m_fileWatchTimer.stop();
    } else {
        m_fileWatchUnblockTimer.start(500);
    }
}

void DatabaseWidget::unblockAutoReload()
{
    m_ignoreAutoReload = false;
    updateFilename(m_filename);
}

void DatabaseWidget::onWatchedFileChanged()
{
    if (m_ignoreAutoReload) {
        return;
    }
    if (m_fileWatchTimer.isActive())
        return;

    m_fileWatchTimer.start(500);
}

void DatabaseWidget::reloadDatabaseFile()
{
    if (m_db == nullptr)
        return;

    if (! config()->get("AutoReloadOnChange").toBool()) {
        // Ask if we want to reload the db
        QMessageBox::StandardButton mb = MessageBox::question(this, tr("Autoreload Request"),
                             tr("The database file has changed. Do you want to load the changes?"),
                             QMessageBox::Yes | QMessageBox::No);

        if (mb == QMessageBox::No) {
            // Notify everyone the database does not match the file
            emit m_db->modified();
            m_databaseModified = true;
            // Rewatch the database file
            m_fileWatcher.addPath(m_filename);
            return;
        }
    }

    KeePass2Reader reader;
    QFile file(m_filename);
    if (file.open(QIODevice::ReadOnly)) {
        Database* db = reader.readDatabase(&file, database()->key());
        if (db != nullptr) {
            if (m_databaseModified) {
                // Ask if we want to merge changes into new database
                QMessageBox::StandardButton mb = MessageBox::question(this, tr("Merge Request"),
                                     tr("The database file has changed and you have unsaved changes."
                                        "Do you want to merge your changes?"),
                                     QMessageBox::Yes | QMessageBox::No);

                if (mb == QMessageBox::Yes) {
                    // Merge the old database into the new one
                    m_db->setEmitModified(false);
                    db->merge(m_db);
                } else {
                    // Since we are accepting the new file as-is, internally mark as unmodified
                    // TODO: when saving is moved out of DatabaseTabWidget, this should be replaced
                    m_databaseModified = false;
                }
            }

            Uuid groupBeforeReload;
            if (m_groupView && m_groupView->currentGroup()) {
                groupBeforeReload = m_groupView->currentGroup()->uuid();
            }
            else {
                groupBeforeReload = m_db->rootGroup()->uuid();
            }

            Uuid entryBeforeReload;
            if (m_entryView && m_entryView->currentEntry()) {
                entryBeforeReload = m_entryView->currentEntry()->uuid();
            }

            replaceDatabase(db);
            restoreGroupEntryFocus(groupBeforeReload, entryBeforeReload);

        }
    } else {
        m_messageWidget->showMessage(
            tr("Could not open the new database file while attempting to autoreload this database."),
            MessageWidget::Error);
    }

    // Rewatch the database file
    m_fileWatcher.addPath(m_filename);
}

int DatabaseWidget::numberOfSelectedEntries() const
{
    return m_entryView->numberOfSelectedEntries();
}

QStringList DatabaseWidget::customEntryAttributes() const
{
    Entry* entry = m_entryView->currentEntry();
    if (!entry) {
        return QStringList();
    }

    return entry->attributes()->customKeys();
}

/*
 * Restores the focus on the group and entry that was focused
 * before the database was locked or reloaded.
 */
void DatabaseWidget::restoreGroupEntryFocus(Uuid groupUuid, Uuid entryUuid)
{
    Group* restoredGroup = nullptr;
    const QList<Group*> groups = m_db->rootGroup()->groupsRecursive(true);
    for (Group* group : groups) {
        if (group->uuid() == groupUuid) {
            restoredGroup = group;
            break;
        }
    }

    if (restoredGroup != nullptr) {
      m_groupView->setCurrentGroup(restoredGroup);

      const QList<Entry*> entries = restoredGroup->entries();
      for (Entry* entry : entries) {
          if (entry->uuid() == entryUuid) {
              m_entryView->setCurrentEntry(entry);
              break;
          }
      }
    }

}

bool DatabaseWidget::isGroupSelected() const
{
    return m_groupView->currentGroup() != nullptr;
}

bool DatabaseWidget::currentEntryHasTitle()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return false;
    }
    return !currentEntry->title().isEmpty();
}

bool DatabaseWidget::currentEntryHasUsername()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return false;
    }
    return !currentEntry->resolveMultiplePlaceholders(currentEntry->username()).isEmpty();
}

bool DatabaseWidget::currentEntryHasPassword()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return false;
    }
    return !currentEntry->resolveMultiplePlaceholders(currentEntry->password()).isEmpty();
}

bool DatabaseWidget::currentEntryHasUrl()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return false;
    }
    return !currentEntry->resolveMultiplePlaceholders(currentEntry->url()).isEmpty();
}


bool DatabaseWidget::currentEntryHasTotp()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return false;
    }
    return currentEntry->hasTotp();
}

bool DatabaseWidget::currentEntryHasNotes()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return false;
    }
    return !currentEntry->resolveMultiplePlaceholders(currentEntry->notes()).isEmpty();
}

GroupView* DatabaseWidget::groupView() {
    return m_groupView;
}

EntryView* DatabaseWidget::entryView() {
    return m_entryView;
}

void DatabaseWidget::showUnlockDialog()
{
    m_unlockDatabaseDialog->clearForms();
    m_unlockDatabaseDialog->setDBFilename(m_filename);
    m_unlockDatabaseDialog->show();
    m_unlockDatabaseDialog->activateWindow();
}

void DatabaseWidget::closeUnlockDialog()
{
    m_unlockDatabaseDialog->close();
}

void DatabaseWidget::showMessage(const QString& text, MessageWidget::MessageType type)
{
    m_messageWidget->showMessage(text, type);
}

void DatabaseWidget::hideMessage()
{
    if (m_messageWidget->isVisible()) {
        m_messageWidget->animatedHide();
    }
}

bool DatabaseWidget::isRecycleBinSelected() const
{
    return m_groupView->currentGroup() && m_groupView->currentGroup() == m_db->metadata()->recycleBin();
}

void DatabaseWidget::emptyRecycleBin()
{
    if(!isRecycleBinSelected()) {
        return;
    }

    QMessageBox::StandardButton result = MessageBox::question(
        this, tr("Empty recycle bin?"),
        tr("Are you sure you want to permanently delete everything from your recycle bin?"),
        QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        m_db->emptyRecycleBin();
        refreshSearch();
    }
}
