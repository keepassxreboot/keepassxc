/*
 * Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
 * Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or (at your option)
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "DatabaseWidget.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDesktopServices>
#include <QFile>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QSplitter>

#include "autotype/AutoType.h"
#include "core/Config.h"
#include "core/Database.h"
#include "core/EntrySearcher.h"
#include "core/FilePath.h"
#include "core/FileWatcher.h"
#include "core/Group.h"
#include "core/Merger.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "format/KeePass2Reader.h"
#include "gui/Clipboard.h"
#include "gui/CloneDialog.h"
#include "gui/DatabaseOpenDialog.h"
#include "gui/DatabaseOpenWidget.h"
#include "gui/EntryPreviewWidget.h"
#include "gui/FileDialog.h"
#include "gui/KeePass1OpenWidget.h"
#include "gui/MessageBox.h"
#include "gui/TotpDialog.h"
#include "gui/TotpExportSettingsDialog.h"
#include "gui/TotpSetupDialog.h"
#include "gui/dbsettings/DatabaseSettingsDialog.h"
#include "gui/entry/EditEntryWidget.h"
#include "gui/entry/EntryView.h"
#include "gui/group/EditGroupWidget.h"
#include "gui/group/GroupView.h"
#include "keeshare/KeeShare.h"
#include "touchid/TouchID.h"

#include "config-keepassx.h"

#ifdef Q_OS_LINUX
#include <sys/vfs.h>
#endif

#ifdef WITH_XC_SSHAGENT
#include "sshagent/SSHAgent.h"
#endif

DatabaseWidget::DatabaseWidget(QSharedPointer<Database> db, QWidget* parent)
    : QStackedWidget(parent)
    , m_db(std::move(db))
    , m_mainWidget(new QWidget(this))
    , m_mainSplitter(new QSplitter(m_mainWidget))
    , m_messageWidget(new MessageWidget(this))
    , m_previewView(new EntryPreviewWidget(this))
    , m_previewSplitter(new QSplitter(m_mainWidget))
    , m_searchingLabel(new QLabel(this))
    , m_csvImportWizard(new CsvImportWizard(this))
    , m_editEntryWidget(new EditEntryWidget(this))
    , m_editGroupWidget(new EditGroupWidget(this))
    , m_historyEditEntryWidget(new EditEntryWidget(this))
    , m_databaseSettingDialog(new DatabaseSettingsDialog(this))
    , m_databaseOpenWidget(new DatabaseOpenWidget(this))
    , m_keepass1OpenWidget(new KeePass1OpenWidget(this))
    , m_groupView(new GroupView(m_db.data(), m_mainSplitter))
    , m_fileWatcher(new DelayingFileWatcher(this))
{
    m_messageWidget->setHidden(true);

    auto* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(m_messageWidget);
    auto* hbox = new QHBoxLayout();
    mainLayout->addLayout(hbox);
    hbox->addWidget(m_mainSplitter);
    m_mainWidget->setLayout(mainLayout);

    auto* rightHandSideWidget = new QWidget(m_mainSplitter);
    auto* vbox = new QVBoxLayout();
    vbox->setMargin(0);
    vbox->addWidget(m_searchingLabel);
    vbox->addWidget(m_previewSplitter);
    rightHandSideWidget->setLayout(vbox);
    m_entryView = new EntryView(rightHandSideWidget);

    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->addWidget(m_groupView);
    m_mainSplitter->addWidget(rightHandSideWidget);
    m_mainSplitter->setStretchFactor(0, 30);
    m_mainSplitter->setStretchFactor(1, 70);

    m_previewSplitter->setOrientation(Qt::Vertical);
    m_previewSplitter->setChildrenCollapsible(true);

    m_groupView->setObjectName("groupView");
    m_groupView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_groupView, SIGNAL(customContextMenuRequested(QPoint)), SLOT(emitGroupContextMenuRequested(QPoint)));

    m_entryView->setObjectName("entryView");
    m_entryView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_entryView->displayGroup(m_db->rootGroup());
    connect(m_entryView, SIGNAL(customContextMenuRequested(QPoint)), SLOT(emitEntryContextMenuRequested(QPoint)));

    // Add a notification for when we are searching
    m_searchingLabel->setText(tr("Searching..."));
    m_searchingLabel->setAlignment(Qt::AlignCenter);
    m_searchingLabel->setStyleSheet("color: rgb(0, 0, 0);"
                                    "background-color: rgb(255, 253, 160);"
                                    "border: 2px solid rgb(190, 190, 190);"
                                    "border-radius: 4px;");
    m_searchingLabel->setVisible(false);

    m_previewView->hide();
    m_previewSplitter->addWidget(m_entryView);
    m_previewSplitter->addWidget(m_previewView);
    m_previewSplitter->setStretchFactor(0, 100);
    m_previewSplitter->setStretchFactor(1, 0);
    m_previewSplitter->setSizes({1, 1});

    m_editEntryWidget->setObjectName("editEntryWidget");
    m_editGroupWidget->setObjectName("editGroupWidget");
    m_csvImportWizard->setObjectName("csvImportWizard");
    m_databaseSettingDialog->setObjectName("databaseSettingsDialog");
    m_databaseOpenWidget->setObjectName("databaseOpenWidget");
    m_keepass1OpenWidget->setObjectName("keepass1OpenWidget");

    addChildWidget(m_mainWidget);
    addChildWidget(m_editEntryWidget);
    addChildWidget(m_editGroupWidget);
    addChildWidget(m_databaseSettingDialog);
    addChildWidget(m_historyEditEntryWidget);
    addChildWidget(m_databaseOpenWidget);
    addChildWidget(m_csvImportWizard);
    addChildWidget(m_keepass1OpenWidget);

    // clang-format off
    connect(m_mainSplitter, SIGNAL(splitterMoved(int,int)), SIGNAL(mainSplitterSizesChanged()));
    connect(m_previewSplitter, SIGNAL(splitterMoved(int,int)), SIGNAL(previewSplitterSizesChanged()));
    connect(this, SIGNAL(currentModeChanged(DatabaseWidget::Mode)), m_previewView, SLOT(setDatabaseMode(DatabaseWidget::Mode)));
    connect(m_previewView, SIGNAL(errorOccurred(QString)), this, SLOT(showErrorMessage(QString)));
    connect(m_entryView, SIGNAL(viewStateChanged()), SIGNAL(entryViewStateChanged()));
    connect(m_groupView, SIGNAL(groupSelectionChanged(Group*)), SLOT(onGroupChanged(Group*)));
    connect(m_groupView, SIGNAL(groupSelectionChanged(Group*)), SIGNAL(groupChanged()));
    connect(m_entryView, SIGNAL(entryActivated(Entry*,EntryModel::ModelColumn)),
        SLOT(entryActivationSignalReceived(Entry*,EntryModel::ModelColumn)));
    connect(m_entryView, SIGNAL(entrySelectionChanged(Entry*)), SLOT(onEntryChanged(Entry*)));
    connect(m_editEntryWidget, SIGNAL(editFinished(bool)), SLOT(switchToMainView(bool)));
    connect(m_editEntryWidget, SIGNAL(historyEntryActivated(Entry*)), SLOT(switchToHistoryView(Entry*)));
    connect(m_historyEditEntryWidget, SIGNAL(editFinished(bool)), SLOT(switchBackToEntryEdit()));
    connect(m_editGroupWidget, SIGNAL(editFinished(bool)), SLOT(switchToMainView(bool)));
    connect(m_databaseSettingDialog, SIGNAL(editFinished(bool)), SLOT(switchToMainView(bool)));
    connect(m_databaseOpenWidget, SIGNAL(dialogFinished(bool)), SLOT(loadDatabase(bool)));
    connect(m_keepass1OpenWidget, SIGNAL(dialogFinished(bool)), SLOT(loadDatabase(bool)));
    connect(m_csvImportWizard, SIGNAL(importFinished(bool)), SLOT(csvImportFinished(bool)));
    connect(m_fileWatcher.data(), SIGNAL(fileChanged()), this, SLOT(reloadDatabaseFile()));
    connect(this, SIGNAL(currentChanged(int)), this, SLOT(emitCurrentModeChanged()));
    // clang-format on

    connectDatabaseSignals();

    m_blockAutoSave = false;

    m_EntrySearcher = new EntrySearcher(false);
    m_searchLimitGroup = config()->get("SearchLimitGroup", false).toBool();

#ifdef WITH_XC_SSHAGENT
    if (config()->get("SSHAgent", false).toBool()) {
        connect(this, SIGNAL(databaseLocked()), SSHAgent::instance(), SLOT(databaseModeChanged()));
        connect(this, SIGNAL(databaseUnlocked()), SSHAgent::instance(), SLOT(databaseModeChanged()));
    }
#endif

    switchToMainView();
}

DatabaseWidget::DatabaseWidget(const QString& filePath, QWidget* parent)
    : DatabaseWidget(QSharedPointer<Database>::create(filePath), parent)
{
}

DatabaseWidget::~DatabaseWidget()
{
    delete m_EntrySearcher;
}

QSharedPointer<Database> DatabaseWidget::database() const
{
    return m_db;
}

DatabaseWidget::Mode DatabaseWidget::currentMode() const
{
    if (currentWidget() == nullptr) {
        return DatabaseWidget::Mode::None;
    } else if (currentWidget() == m_mainWidget) {
        return DatabaseWidget::Mode::ViewMode;
    } else if (currentWidget() == m_databaseOpenWidget || currentWidget() == m_keepass1OpenWidget) {
        return DatabaseWidget::Mode::LockedMode;
    } else if (currentWidget() == m_csvImportWizard) {
        return DatabaseWidget::Mode::ImportMode;
    } else {
        return DatabaseWidget::Mode::EditMode;
    }
}

bool DatabaseWidget::isLocked() const
{
    return currentMode() == Mode::LockedMode;
}

bool DatabaseWidget::isSearchActive() const
{
    return m_entryView->inSearchMode();
}

bool DatabaseWidget::isEditWidgetModified() const
{
    if (currentWidget() == m_editEntryWidget) {
        return m_editEntryWidget->hasBeenModified();
    } else {
        // other edit widget don't have a hasBeenModified() method yet
        // assume that they already have been modified
        return true;
    }
}

QList<int> DatabaseWidget::mainSplitterSizes() const
{
    return m_mainSplitter->sizes();
}

void DatabaseWidget::setMainSplitterSizes(const QList<int>& sizes)
{
    m_mainSplitter->setSizes(sizes);
}

QList<int> DatabaseWidget::previewSplitterSizes() const
{
    return m_previewSplitter->sizes();
}

void DatabaseWidget::setPreviewSplitterSizes(const QList<int>& sizes)
{
    m_previewSplitter->setSizes(sizes);
}

/**
 * Get current state of entry view 'Hide Usernames' setting
 */
bool DatabaseWidget::isUsernamesHidden() const
{
    return m_entryView->isUsernamesHidden();
}

/**
 * Set state of entry view 'Hide Usernames' setting
 */
void DatabaseWidget::setUsernamesHidden(bool hide)
{
    m_entryView->setUsernamesHidden(hide);
}

/**
 * Get current state of entry view 'Hide Passwords' setting
 */
bool DatabaseWidget::isPasswordsHidden() const
{
    return m_entryView->isPasswordsHidden();
}

/**
 * Set state of entry view 'Hide Passwords' setting
 */
void DatabaseWidget::setPasswordsHidden(bool hide)
{
    m_entryView->setPasswordsHidden(hide);
}

/**
 * Get current view state of entry view
 */
QByteArray DatabaseWidget::entryViewState() const
{
    return m_entryView->viewState();
}

/**
 * Set view state of entry view
 */
bool DatabaseWidget::setEntryViewState(const QByteArray& state) const
{
    return m_entryView->setViewState(state);
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

void DatabaseWidget::createEntry()
{
    Q_ASSERT(m_groupView->currentGroup());
    if (!m_groupView->currentGroup()) {
        return;
    }

    m_newEntry.reset(new Entry());

    if (isSearchActive()) {
        m_newEntry->setTitle(getCurrentSearch());
        endSearch();
    }
    m_newEntry->setUuid(QUuid::createUuid());
    m_newEntry->setUsername(m_db->metadata()->defaultUserName());
    m_newParent = m_groupView->currentGroup();
    setIconFromParent();
    switchToEntryEdit(m_newEntry.data(), true);
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
    } else {
        m_newEntry->setIcon(m_newParent->iconUuid());
    }
}

void DatabaseWidget::replaceDatabase(QSharedPointer<Database> db)
{
    // TODO: instead of increasing the ref count temporarily, there should be a clean
    // break from the old database. Without this crashes occur due to the change
    // signals triggering dangling pointers.
    auto oldDb = m_db;
    m_db = std::move(db);
    connectDatabaseSignals();
    m_groupView->changeDatabase(m_db);
    processAutoOpen();
#if defined(WITH_XC_KEESHARE)
    KeeShare::instance()->connectDatabase(m_db, oldDb);
#endif
}

void DatabaseWidget::cloneEntry()
{
    Entry* currentEntry = m_entryView->currentEntry();
    Q_ASSERT(currentEntry);
    if (!currentEntry) {
        return;
    }

    auto cloneDialog = new CloneDialog(this, m_db.data(), currentEntry);
    cloneDialog->show();
}

void DatabaseWidget::showTotp()
{
    Entry* currentEntry = m_entryView->currentEntry();
    Q_ASSERT(currentEntry);
    if (!currentEntry) {
        return;
    }

    auto totpDialog = new TotpDialog(this, currentEntry);
    totpDialog->open();
}

void DatabaseWidget::copyTotp()
{
    Entry* currentEntry = m_entryView->currentEntry();
    Q_ASSERT(currentEntry);
    if (!currentEntry) {
        return;
    }
    setClipboardTextAndMinimize(currentEntry->totp());
}

void DatabaseWidget::setupTotp()
{
    Entry* currentEntry = m_entryView->currentEntry();
    Q_ASSERT(currentEntry);
    if (!currentEntry) {
        return;
    }

    auto setupTotpDialog = new TotpSetupDialog(this, currentEntry);
    connect(setupTotpDialog, SIGNAL(totpUpdated()), SIGNAL(entrySelectionChanged()));
    setupTotpDialog->open();
}

void DatabaseWidget::deleteSelectedEntries()
{
    const QModelIndexList selected = m_entryView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return;
    }

    // Resolve entries from the selection model
    QList<Entry*> selectedEntries;
    for (const QModelIndex& index : selected) {
        selectedEntries.append(m_entryView->entryFromIndex(index));
    }

    // Confirm entry removal before moving forward
    auto* recycleBin = m_db->metadata()->recycleBin();
    bool permanent = (recycleBin && recycleBin->findEntryByUuid(selectedEntries.first()->uuid()))
                     || !m_db->metadata()->recycleBinEnabled();

    if (!confirmDeleteEntries(selectedEntries, permanent)) {
        return;
    }

    // Find references to selected entries and prompt for direction if necessary
    auto it = selectedEntries.begin();
    while (it != selectedEntries.end()) {
        auto references = m_db->rootGroup()->referencesRecursive(*it);
        if (!references.isEmpty()) {
            // Ignore references that are selected for deletion
            for (auto* entry : selectedEntries) {
                references.removeAll(entry);
            }

            if (!references.isEmpty()) {
                // Prompt for reference handling
                auto result = MessageBox::question(
                    this,
                    tr("Replace references to entry?"),
                    tr("Entry \"%1\" has %2 reference(s). "
                       "Do you want to overwrite references with values, skip this entry, or delete anyway?",
                       "",
                       references.size())
                        .arg((*it)->title().toHtmlEscaped())
                        .arg(references.size()),
                    MessageBox::Overwrite | MessageBox::Skip | MessageBox::Delete,
                    MessageBox::Overwrite);

                if (result == MessageBox::Overwrite) {
                    for (auto* entry : references) {
                        entry->replaceReferencesWithValues(*it);
                    }
                } else if (result == MessageBox::Skip) {
                    it = selectedEntries.erase(it);
                    continue;
                }
            }
        }

        it++;
    }

    if (permanent) {
        for (auto* entry : asConst(selectedEntries)) {
            delete entry;
        }
    } else {
        for (auto* entry : asConst(selectedEntries)) {
            m_db->recycleEntry(entry);
        }
    }

    refreshSearch();
}

bool DatabaseWidget::confirmDeleteEntries(QList<Entry*> entries, bool permanent)
{
    if (entries.isEmpty()) {
        return false;
    }

    if (permanent) {
        QString prompt;
        if (entries.size() == 1) {
            prompt = tr("Do you really want to delete the entry \"%1\" for good?")
                         .arg(entries.first()->title().toHtmlEscaped());
        } else {
            prompt = tr("Do you really want to delete %n entry(s) for good?", "", entries.size());
        }

        auto answer = MessageBox::question(this,
                                           tr("Delete entry(s)?", "", entries.size()),
                                           prompt,
                                           MessageBox::Delete | MessageBox::Cancel,
                                           MessageBox::Cancel);

        return answer == MessageBox::Delete;
    } else {
        QString prompt;
        if (entries.size() == 1) {
            prompt = tr("Do you really want to move entry \"%1\" to the recycle bin?")
                         .arg(entries.first()->title().toHtmlEscaped());
        } else {
            prompt = tr("Do you really want to move %n entry(s) to the recycle bin?", "", entries.size());
        }

        auto answer = MessageBox::question(this,
                                           tr("Move entry(s) to recycle bin?", "", entries.size()),
                                           prompt,
                                           MessageBox::Move | MessageBox::Cancel,
                                           MessageBox::Cancel);

        return answer == MessageBox::Move;
    }
}

void DatabaseWidget::setFocus()
{
    m_entryView->setFocus();
}

void DatabaseWidget::copyTitle()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (currentEntry) {
        setClipboardTextAndMinimize(currentEntry->resolveMultiplePlaceholders(currentEntry->title()));
    }
}

void DatabaseWidget::copyUsername()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (currentEntry) {
        setClipboardTextAndMinimize(currentEntry->resolveMultiplePlaceholders(currentEntry->username()));
    }
}

void DatabaseWidget::copyPassword()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (currentEntry) {
        setClipboardTextAndMinimize(currentEntry->resolveMultiplePlaceholders(currentEntry->password()));
    }
}

void DatabaseWidget::copyURL()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (currentEntry) {
        setClipboardTextAndMinimize(currentEntry->resolveMultiplePlaceholders(currentEntry->url()));
    }
}

void DatabaseWidget::copyNotes()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (currentEntry) {
        setClipboardTextAndMinimize(currentEntry->resolveMultiplePlaceholders(currentEntry->notes()));
    }
}

void DatabaseWidget::copyAttribute(QAction* action)
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (currentEntry) {
        setClipboardTextAndMinimize(
            currentEntry->resolveMultiplePlaceholders(currentEntry->attributes()->value(action->data().toString())));
    }
}

void DatabaseWidget::showTotpKeyQrCode()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (currentEntry) {
        auto totpDisplayDialog = new TotpExportSettingsDialog(this, currentEntry);
        totpDisplayDialog->open();
    }
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
    if (currentEntry) {
        autoType()->performAutoType(currentEntry, window());
    }
}

void DatabaseWidget::openUrl()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (currentEntry) {
        openUrlForEntry(currentEntry);
    }
}

void DatabaseWidget::openUrlForEntry(Entry* entry)
{
    Q_ASSERT(entry);
    QString cmdString = entry->resolveMultiplePlaceholders(entry->url());
    if (cmdString.startsWith("cmd://")) {
        // check if decision to execute command was stored
        if (entry->attributes()->hasKey(EntryAttributes::RememberCmdExecAttr)) {
            if (entry->attributes()->value(EntryAttributes::RememberCmdExecAttr) == "1") {
                QProcess::startDetached(cmdString.mid(6));
            }
            return;
        }

        // otherwise ask user
        if (cmdString.length() > 6) {
            QString cmdTruncated = cmdString.mid(6);
            if (cmdTruncated.length() > 400) {
                cmdTruncated = cmdTruncated.left(400) + " […]";
            }
            QMessageBox msgbox(QMessageBox::Icon::Question,
                               tr("Execute command?"),
                               tr("Do you really want to execute the following command?<br><br>%1<br>")
                                   .arg(cmdTruncated.toHtmlEscaped()),
                               QMessageBox::Yes | QMessageBox::No,
                               this);
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
                QProcess::startDetached(cmdString.mid(6));
            }

            if (remember) {
                entry->attributes()->set(EntryAttributes::RememberCmdExecAttr, result == QMessageBox::Yes ? "1" : "0");
            }
        }
    } else {
        QString urlString = entry->webUrl();
        if (!urlString.isEmpty()) {
            QDesktopServices::openUrl(urlString);
        }
    }
}

void DatabaseWidget::createGroup()
{
    Q_ASSERT(m_groupView->currentGroup());
    if (!m_groupView->currentGroup()) {
        return;
    }

    m_newGroup.reset(new Group());
    m_newGroup->setUuid(QUuid::createUuid());
    m_newParent = m_groupView->currentGroup();
    switchToGroupEdit(m_newGroup.data(), true);
}

void DatabaseWidget::deleteGroup()
{
    Group* currentGroup = m_groupView->currentGroup();
    Q_ASSERT(currentGroup && canDeleteCurrentGroup());
    if (!currentGroup || !canDeleteCurrentGroup()) {
        return;
    }

    auto* recycleBin = m_db->metadata()->recycleBin();
    bool inRecycleBin = recycleBin && recycleBin->findGroupByUuid(currentGroup->uuid());
    bool isRecycleBin = recycleBin && (currentGroup == recycleBin);
    bool isRecycleBinSubgroup = recycleBin && currentGroup->findGroupByUuid(recycleBin->uuid());
    if (inRecycleBin || isRecycleBin || isRecycleBinSubgroup || !m_db->metadata()->recycleBinEnabled()) {
        auto result = MessageBox::question(
            this,
            tr("Delete group"),
            tr("Do you really want to delete the group \"%1\" for good?").arg(currentGroup->name().toHtmlEscaped()),
            MessageBox::Delete | MessageBox::Cancel,
            MessageBox::Cancel);

        if (result == MessageBox::Delete) {
            delete currentGroup;
        }
    } else {
        auto result = MessageBox::question(this,
                                           tr("Move group to recycle bin?"),
                                           tr("Do you really want to move the group "
                                              "\"%1\" to the recycle bin?")
                                               .arg(currentGroup->name().toHtmlEscaped()),
                                           MessageBox::Move | MessageBox::Cancel,
                                           MessageBox::Cancel);
        if (result == MessageBox::Move) {
            m_db->recycleGroup(currentGroup);
        }
    }
}

int DatabaseWidget::addChildWidget(QWidget* w)
{
    w->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    int index = QStackedWidget::addWidget(w);
    adjustSize();
    return index;
}

void DatabaseWidget::switchToMainView(bool previousDialogAccepted)
{
    if (m_newGroup) {
        if (previousDialogAccepted) {
            m_newGroup->setParent(m_newParent);
            m_groupView->setCurrentGroup(m_newGroup.take());
            m_groupView->expandGroup(m_newParent);
        } else {
            m_newGroup.reset();
        }

        m_newParent = nullptr;
    } else if (m_newEntry) {
        if (previousDialogAccepted) {
            m_newEntry->setGroup(m_newParent);
            m_entryView->setFocus();
            m_entryView->setCurrentEntry(m_newEntry.take());
        } else {
            m_newEntry.reset();
        }

        m_newParent = nullptr;
    }

    setCurrentWidget(m_mainWidget);

    if (sender() == m_entryView) {
        onEntryChanged(m_entryView->currentEntry());
    } else if (sender() == m_groupView) {
        onGroupChanged(m_groupView->currentGroup());
    }
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
    // If creating an entry, it will be in `currentGroup()` so it's
    // okay to use but when editing, the entry may not be in
    // `currentGroup()` so we get the entry's group.
    Group* group;
    if (create) {
        group = currentGroup();
    } else {
        group = entry->group();
    }

    Q_ASSERT(group);

    m_editEntryWidget->loadEntry(entry, create, false, group->name(), m_db);
    setCurrentWidget(m_editEntryWidget);
}

void DatabaseWidget::switchToGroupEdit(Group* group, bool create)
{
    m_editGroupWidget->loadGroup(group, create, m_db);
    setCurrentWidget(m_editGroupWidget);
}

void DatabaseWidget::connectDatabaseSignals()
{
    // relayed Database events
    connect(m_db.data(),
            SIGNAL(filePathChanged(QString, QString)),

            SIGNAL(databaseFilePathChanged(QString, QString)));
    connect(m_db.data(), SIGNAL(databaseModified()), SIGNAL(databaseModified()));
    connect(m_db.data(), SIGNAL(databaseModified()), SLOT(onDatabaseModified()));
    connect(m_db.data(), SIGNAL(databaseSaved()), SIGNAL(databaseSaved()));
}

void DatabaseWidget::loadDatabase(bool accepted)
{
    auto* openWidget = qobject_cast<DatabaseOpenWidget*>(sender());
    Q_ASSERT(openWidget);
    if (!openWidget) {
        return;
    }

    if (accepted) {
        replaceDatabase(openWidget->database());
        switchToMainView();
        m_fileWatcher->restart();
        emit databaseUnlocked();
    } else {
        m_fileWatcher->stop();
        if (m_databaseOpenWidget->database()) {
            m_databaseOpenWidget->database().reset();
        }
        emit closeRequest();
    }
}

void DatabaseWidget::mergeDatabase(bool accepted)
{
    if (accepted) {
        if (!m_db) {
            showMessage(tr("No current database."), MessageWidget::Error);
            return;
        }

        auto* senderDialog = qobject_cast<DatabaseOpenDialog*>(sender());

        Q_ASSERT(senderDialog);
        if (!senderDialog) {
            return;
        }
        auto srcDb = senderDialog->database();

        if (!srcDb) {
            showMessage(tr("No source database, nothing to do."), MessageWidget::Error);
            return;
        }

        Merger merger(srcDb.data(), m_db.data());
        bool databaseChanged = merger.merge();

        if (databaseChanged) {
            showMessage(tr("Successfully merged the database files."), MessageWidget::Information);
        } else {
            showMessage(tr("Database was not modified by merge operation."), MessageWidget::Information);
        }
    }

    switchToMainView();
    emit databaseMerged(m_db);
}

/**
 * Unlock the database.
 *
 * @param accepted true if the unlock dialog or widget was confirmed with OK
 */
void DatabaseWidget::unlockDatabase(bool accepted)
{
    auto* senderDialog = qobject_cast<DatabaseOpenDialog*>(sender());

    if (!accepted) {
        if (!senderDialog && (!m_db || !m_db->isInitialized())) {
            emit closeRequest();
        }
        return;
    }

    if (senderDialog && senderDialog->intent() == DatabaseOpenDialog::Intent::Merge) {
        mergeDatabase(accepted);
        return;
    }

    QSharedPointer<Database> db;
    if (senderDialog) {
        db = senderDialog->database();
    } else {
        db = m_databaseOpenWidget->database();
    }
    replaceDatabase(db);
    if (db->isReadOnly()) {
        showMessage(tr("File opened in read only mode."), MessageWidget::Warning, false, -1);
    }

    restoreGroupEntryFocus(m_groupBeforeLock, m_entryBeforeLock);
    m_groupBeforeLock = QUuid();
    m_entryBeforeLock = QUuid();

    switchToMainView();
    emit databaseUnlocked();

    if (senderDialog && senderDialog->intent() == DatabaseOpenDialog::Intent::AutoType) {
        QList<QSharedPointer<Database>> dbList;
        dbList.append(m_db);
        autoType()->performGlobalAutoType(dbList);
    }
}

void DatabaseWidget::entryActivationSignalReceived(Entry* entry, EntryModel::ModelColumn column)
{
    Q_ASSERT(entry);
    if (!entry) {
        return;
    }

    // Implement 'copy-on-doubleclick' functionality for certain columns
    switch (column) {
    case EntryModel::Username:
        setClipboardTextAndMinimize(entry->resolveMultiplePlaceholders(entry->username()));
        break;
    case EntryModel::Password:
        setClipboardTextAndMinimize(entry->resolveMultiplePlaceholders(entry->password()));
        break;
    case EntryModel::Url:
        if (!entry->url().isEmpty()) {
            openUrlForEntry(entry);
        }
        break;
    case EntryModel::Totp:
        if (entry->hasTotp()) {
            setClipboardTextAndMinimize(entry->totp());
        } else {
            setupTotp();
        }
        break;
    case EntryModel::ParentGroup:
        // Call this first to clear out of search mode, otherwise
        // the desired entry is not properly selected
        endSearch();
        m_groupView->setCurrentGroup(entry->group());
        m_entryView->setCurrentEntry(entry);
        break;
    // TODO: switch to 'Notes' tab in details view/pane
    // case EntryModel::Notes:
    //    break;
    // TODO: switch to 'Attachments' tab in details view/pane
    // case EntryModel::Attachments:
    //    break;
    default:
        switchToEntryEdit(entry);
    }
}

void DatabaseWidget::switchToDatabaseSettings()
{
    m_databaseSettingDialog->load(m_db);
    setCurrentWidget(m_databaseSettingDialog);
}

void DatabaseWidget::switchToOpenDatabase()
{
    switchToOpenDatabase(m_db->filePath());
}

void DatabaseWidget::switchToOpenDatabase(const QString& filePath)
{
    updateFilePath(filePath);
    m_databaseOpenWidget->load(filePath);
    setCurrentWidget(m_databaseOpenWidget);
}

void DatabaseWidget::switchToOpenDatabase(const QString& filePath, const QString& password, const QString& keyFile)
{
    switchToOpenDatabase(filePath);
    m_databaseOpenWidget->enterKey(password, keyFile);
}

void DatabaseWidget::switchToCsvImport(const QString& filePath)
{
    setCurrentWidget(m_csvImportWizard);
    m_csvImportWizard->load(filePath, m_db.data());
}

void DatabaseWidget::csvImportFinished(bool accepted)
{
    if (!accepted) {
        emit closeRequest();
    } else {
        switchToMainView();
    }
}

void DatabaseWidget::switchToImportKeepass1(const QString& filePath)
{
    updateFilePath(filePath);
    m_keepass1OpenWidget->load(filePath);
    setCurrentWidget(m_keepass1OpenWidget);
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

void DatabaseWidget::switchToMasterKeyChange()
{
    switchToDatabaseSettings();
    m_databaseSettingDialog->showMasterKeySettings();
}

void DatabaseWidget::performUnlockDatabase(const QString& password, const QString& keyfile)
{
    if (password.isEmpty() && keyfile.isEmpty()) {
        return;
    }

    if (!m_db->isInitialized() || isLocked()) {
        switchToOpenDatabase();
        m_databaseOpenWidget->enterKey(password, keyfile);
    }
}

void DatabaseWidget::refreshSearch()
{
    if (isSearchActive()) {
        search(m_lastSearchText);
    }
}

void DatabaseWidget::search(const QString& searchtext)
{
    if (searchtext.isEmpty()) {
        endSearch();
        return;
    }

    emit searchModeAboutToActivate();

    Group* searchGroup = m_searchLimitGroup ? currentGroup() : m_db->rootGroup();

    QList<Entry*> searchResult = m_EntrySearcher->search(searchtext, searchGroup);

    m_entryView->displaySearch(searchResult);
    m_lastSearchText = searchtext;

    // Display a label detailing our search results
    if (!searchResult.isEmpty()) {
        m_searchingLabel->setText(tr("Search Results (%1)").arg(searchResult.size()));
    } else {
        m_searchingLabel->setText(tr("No Results"));
    }

    m_searchingLabel->setVisible(true);

    emit searchModeActivated();
}

void DatabaseWidget::setSearchCaseSensitive(bool state)
{
    m_EntrySearcher->setCaseSensitive(state);
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
    if (isSearchActive() && m_searchLimitGroup) {
        search(m_lastSearchText);
    } else if (isSearchActive()) {
        endSearch();
    } else {
        m_entryView->displayGroup(group);
    }

    m_previewView->setGroup(group);
}

void DatabaseWidget::onDatabaseModified()
{
    if (!m_blockAutoSave && config()->get("AutoSaveAfterEveryChange").toBool()) {
        save();
    }

    m_blockAutoSave = false;
}

QString DatabaseWidget::getCurrentSearch()
{
    return m_lastSearchText;
}

void DatabaseWidget::endSearch()
{
    if (isSearchActive()) {
        emit listModeAboutToActivate();

        // Show the normal entry view of the current group
        m_entryView->displayGroup(currentGroup());

        emit listModeActivated();
    }

    m_searchingLabel->setVisible(false);
    m_searchingLabel->setText(tr("Searching..."));

    m_lastSearchText.clear();

    // Tell the search widget to clear
    emit clearSearch();
}

void DatabaseWidget::emitGroupContextMenuRequested(const QPoint& pos)
{
    emit groupContextMenuRequested(m_groupView->viewport()->mapToGlobal(pos));
}

void DatabaseWidget::emitEntryContextMenuRequested(const QPoint& pos)
{
    emit entryContextMenuRequested(m_entryView->viewport()->mapToGlobal(pos));
}

void DatabaseWidget::onEntryChanged(Entry* entry)
{
    if (entry) {
        m_previewView->setEntry(entry);
    }

    emit entrySelectionChanged();
}

bool DatabaseWidget::canDeleteCurrentGroup() const
{
    bool isRootGroup = m_db->rootGroup() == m_groupView->currentGroup();
    return !isRootGroup;
}

Group* DatabaseWidget::currentGroup() const
{
    return m_groupView->currentGroup();
}

void DatabaseWidget::closeEvent(QCloseEvent* event)
{
    if (!isLocked() && !lock()) {
        event->ignore();
        return;
    }

    event->accept();
}

void DatabaseWidget::showEvent(QShowEvent* event)
{
    if (!m_db->isInitialized() || isLocked()) {
        switchToOpenDatabase();
    }

    event->accept();
}

bool DatabaseWidget::lock()
{
    if (isLocked()) {
        return true;
    }

    clipboard()->clearCopiedText();

    if (currentMode() == DatabaseWidget::Mode::EditMode) {
        auto result = MessageBox::question(this,
                                           tr("Lock Database?"),
                                           tr("You are editing an entry. Discard changes and lock anyway?"),
                                           MessageBox::Discard | MessageBox::Cancel,
                                           MessageBox::Cancel);
        if (result == MessageBox::Cancel) {
            return false;
        }
    }

    if (m_db->isModified()) {
        if (config()->get("AutoSaveOnExit").toBool()) {
            if (!save()) {
                return false;
            }
        } else {
            QString msg;
            if (!m_db->metadata()->name().toHtmlEscaped().isEmpty()) {
                msg = tr("\"%1\" was modified.\nSave changes?").arg(m_db->metadata()->name().toHtmlEscaped());
            } else {
                msg = tr("Database was modified.\nSave changes?");
            }
            auto result = MessageBox::question(this,
                                               tr("Save changes?"),
                                               msg,
                                               MessageBox::Save | MessageBox::Discard | MessageBox::Cancel,
                                               MessageBox::Save);
            if (result == MessageBox::Save) {
                if (!save()) {
                    return false;
                }
            } else if (result == MessageBox::Cancel) {
                return false;
            }
        }
    }

    if (m_groupView->currentGroup()) {
        m_groupBeforeLock = m_groupView->currentGroup()->uuid();
    } else {
        m_groupBeforeLock = m_db->rootGroup()->uuid();
    }

    if (m_entryView->currentEntry()) {
        m_entryBeforeLock = m_entryView->currentEntry()->uuid();
    }

    endSearch();
    clearAllWidgets();
    switchToOpenDatabase(m_db->filePath());

    auto newDb = QSharedPointer<Database>::create(m_db->filePath());
    replaceDatabase(newDb);

    emit databaseLocked();

    return true;
}

void DatabaseWidget::updateFilePath(const QString& filePath)
{
    m_fileWatcher->start(filePath);
    m_db->setFilePath(filePath);
}

void DatabaseWidget::blockAutoReload(bool block)
{
    if (block) {
        m_fileWatcher->ignoreFileChanges();
    } else {
        m_fileWatcher->observeFileChanges(true);
    }
}

void DatabaseWidget::reloadDatabaseFile()
{
    if (!m_db || isLocked()) {
        return;
    }

    m_blockAutoSave = true;

    if (!config()->get("AutoReloadOnChange").toBool()) {
        // Ask if we want to reload the db
        auto result = MessageBox::question(this,
                                           tr("File has changed"),
                                           tr("The database file has changed. Do you want to load the changes?"),
                                           MessageBox::Yes | MessageBox::No);

        if (result == MessageBox::No) {
            // Notify everyone the database does not match the file
            m_db->markAsModified();
            // Rewatch the database file
            m_fileWatcher->restart();
            return;
        }
    }

    QString error;
    auto db = QSharedPointer<Database>::create(m_db->filePath());
    if (db->open(database()->key(), &error, true)) {
        if (m_db->isModified()) {
            // Ask if we want to merge changes into new database
            auto result = MessageBox::question(
                this,
                tr("Merge Request"),
                tr("The database file has changed and you have unsaved changes.\nDo you want to merge your changes?"),
                MessageBox::Merge | MessageBox::Cancel,
                MessageBox::Merge);

            if (result == MessageBox::Merge) {
                // Merge the old database into the new one
                Merger merger(m_db.data(), db.data());
                merger.merge();
            }
        }

        QUuid groupBeforeReload;
        if (m_groupView && m_groupView->currentGroup()) {
            groupBeforeReload = m_groupView->currentGroup()->uuid();
        } else {
            groupBeforeReload = m_db->rootGroup()->uuid();
        }

        QUuid entryBeforeReload;
        if (m_entryView && m_entryView->currentEntry()) {
            entryBeforeReload = m_entryView->currentEntry()->uuid();
        }

        bool isReadOnly = m_db->isReadOnly();
        replaceDatabase(db);
        m_db->setReadOnly(isReadOnly);
        restoreGroupEntryFocus(groupBeforeReload, entryBeforeReload);
    } else {
        showMessage(tr("Could not open the new database file while attempting to autoreload.\nError: %1").arg(error),
                    MessageWidget::Error);
        // Mark db as modified since existing data may differ from file or file was deleted
        m_db->markAsModified();
    }

    // Rewatch the database file
    m_fileWatcher->restart();
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
 * Restores the focus on the group and entry provided
 */
void DatabaseWidget::restoreGroupEntryFocus(const QUuid& groupUuid, const QUuid& entryUuid)
{
    auto group = m_db->rootGroup()->findGroupByUuid(groupUuid);
    if (group) {
        m_groupView->setCurrentGroup(group);
        auto entry = group->findEntryByUuid(entryUuid);
        if (entry) {
            m_entryView->setCurrentEntry(entry);
        }
    }
}

bool DatabaseWidget::isGroupSelected() const
{
    return m_groupView->currentGroup();
}

bool DatabaseWidget::currentEntryHasFocus()
{
    return m_entryView->numberOfSelectedEntries() > 0 && m_entryView->hasFocus();
}

bool DatabaseWidget::currentEntryHasTitle()
{
    Entry* currentEntry = m_entryView->currentEntry();
    Q_ASSERT(currentEntry);
    if (!currentEntry) {
        return false;
    }
    return !currentEntry->title().isEmpty();
}

bool DatabaseWidget::currentEntryHasUsername()
{
    Entry* currentEntry = m_entryView->currentEntry();
    Q_ASSERT(currentEntry);
    if (!currentEntry) {
        return false;
    }
    return !currentEntry->resolveMultiplePlaceholders(currentEntry->username()).isEmpty();
}

bool DatabaseWidget::currentEntryHasPassword()
{
    Entry* currentEntry = m_entryView->currentEntry();
    Q_ASSERT(currentEntry);
    if (!currentEntry) {
        return false;
    }
    return !currentEntry->resolveMultiplePlaceholders(currentEntry->password()).isEmpty();
}

bool DatabaseWidget::currentEntryHasUrl()
{
    Entry* currentEntry = m_entryView->currentEntry();
    Q_ASSERT(currentEntry);
    if (!currentEntry) {
        return false;
    }
    return !currentEntry->resolveMultiplePlaceholders(currentEntry->url()).isEmpty();
}

bool DatabaseWidget::currentEntryHasTotp()
{
    Entry* currentEntry = m_entryView->currentEntry();
    Q_ASSERT(currentEntry);
    if (!currentEntry) {
        return false;
    }
    return currentEntry->hasTotp();
}

bool DatabaseWidget::currentEntryHasNotes()
{
    Entry* currentEntry = m_entryView->currentEntry();
    Q_ASSERT(currentEntry);
    if (!currentEntry) {
        return false;
    }
    return !currentEntry->resolveMultiplePlaceholders(currentEntry->notes()).isEmpty();
}

GroupView* DatabaseWidget::groupView()
{
    return m_groupView;
}

EntryView* DatabaseWidget::entryView()
{
    return m_entryView;
}

/**
 * Save the database to disk.
 *
 * This method will try to save several times in case of failure and
 * ask to disable safe saves if it is unable to save after the third attempt.
 * Set `attempt` to -1 to disable this behavior.
 *
 * @param attempt current save attempt or -1 to disable attempts
 * @return true on success
 */
bool DatabaseWidget::save(int attempt)
{
    // Never allow saving a locked database; it causes corruption
    Q_ASSERT(!isLocked());
    // Release build interlock
    if (isLocked()) {
        // We return true since a save is not required
        return true;
    }

    if (m_db->isReadOnly() || m_db->filePath().isEmpty()) {
        return saveAs();
    }

    blockAutoReload(true);
    // TODO: Make this async, but lock out the database widget to prevent re-entrance
    bool useAtomicSaves = config()->get("UseAtomicSaves", true).toBool();
    QString errorMessage;
    bool ok = m_db->save(&errorMessage, useAtomicSaves, config()->get("BackupBeforeSave").toBool());
    blockAutoReload(false);

    if (ok) {
        return true;
    }

    if (attempt >= 0 && attempt <= 2) {
        return save(attempt + 1);
    }

    if (attempt > 2 && useAtomicSaves) {
        // Saving failed 3 times, issue a warning and attempt to resolve
        auto result = MessageBox::question(this,
                                           tr("Disable safe saves?"),
                                           tr("KeePassXC has failed to save the database multiple times. "
                                              "This is likely caused by file sync services holding a lock on "
                                              "the save file.\nDisable safe saves and try again?"),
                                           MessageBox::Disable | MessageBox::Cancel,
                                           MessageBox::Disable);
        if (result == MessageBox::Disable) {
            config()->set("UseAtomicSaves", false);
            return save(attempt + 1);
        }
    }

    showMessage(tr("Writing the database failed.\n%1").arg(errorMessage), MessageWidget::Error);
    return false;
}

/**
 * Save database under a new user-selected filename.
 *
 * @return true on success
 */
bool DatabaseWidget::saveAs()
{
    while (true) {
        QString oldFilePath = m_db->filePath();
        if (!QFileInfo(oldFilePath).exists()) {
            oldFilePath = QDir::toNativeSeparators(config()->get("LastDir", QDir::homePath()).toString() + "/"
                                                   + tr("Passwords").append(".kdbx"));
        }
        QString newFilePath = fileDialog()->getSaveFileName(this,
                                                            tr("Save database as"),
                                                            oldFilePath,
                                                            tr("KeePass 2 Database").append(" (*.kdbx)"),
                                                            nullptr,
                                                            nullptr,
                                                            "kdbx");

        if (!newFilePath.isEmpty()) {
            // Ensure we don't recurse back into this function
            m_db->setReadOnly(false);
            m_db->setFilePath(newFilePath);

            if (!save(-1)) {
                // Failed to save, try again
                continue;
            }

            return true;
        }

        // Canceled file selection
        return false;
    }
}

void DatabaseWidget::showMessage(const QString& text,
                                 MessageWidget::MessageType type,
                                 bool showClosebutton,
                                 int autoHideTimeout)
{
    m_messageWidget->setCloseButtonVisible(showClosebutton);
    m_messageWidget->showMessage(text, type, autoHideTimeout);
}

void DatabaseWidget::showErrorMessage(const QString& errorMessage)
{
    showMessage(errorMessage, MessageWidget::MessageType::Error);
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
    if (!isRecycleBinSelected()) {
        return;
    }

    auto result =
        MessageBox::question(this,
                             tr("Empty recycle bin?"),
                             tr("Are you sure you want to permanently delete everything from your recycle bin?"),
                             MessageBox::Empty | MessageBox::Cancel,
                             MessageBox::Cancel);

    if (result == MessageBox::Empty) {
        m_db->emptyRecycleBin();
        refreshSearch();
    }
}

void DatabaseWidget::processAutoOpen()
{
    Q_ASSERT(m_db);

    auto* autoopenGroup = m_db->rootGroup()->findGroupByPath("/AutoOpen");
    if (!autoopenGroup) {
        return;
    }

    for (const auto* entry : autoopenGroup->entries()) {
        if (entry->url().isEmpty() || entry->password().isEmpty()) {
            continue;
        }
        QFileInfo filepath;
        if (entry->url().startsWith("file://")) {
            QUrl url(entry->url());
            filepath.setFile(url.toLocalFile());
        } else {
            filepath.setFile(entry->url());
            if (filepath.isRelative()) {
                QFileInfo currentpath(m_db->filePath());
                filepath.setFile(currentpath.absoluteDir(), entry->url());
            }
        }

        if (!filepath.isFile()) {
            continue;
        }

        // Request to open the database file in the background
        emit requestOpenDatabase(filepath.canonicalFilePath(), true, entry->password());
    }
}
