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

#include "DatabaseWidget.h"
#include "ui_SearchWidget.h"

#include <QtCore/QTimer>
#include <QtGui/QAction>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QSplitter>

#include "autotype/AutoType.h"
#include "core/FilePath.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "gui/ChangeMasterKeyWidget.h"
#include "gui/Clipboard.h"
#include "gui/DatabaseOpenWidget.h"
#include "gui/DatabaseSettingsWidget.h"
#include "gui/KeePass1OpenWidget.h"
#include "gui/entry/EditEntryWidget.h"
#include "gui/entry/EntryView.h"
#include "gui/group/EditGroupWidget.h"
#include "gui/group/GroupView.h"

DatabaseWidget::DatabaseWidget(Database* db, QWidget* parent)
    : QStackedWidget(parent)
    , m_db(db)
    , m_searchUi(new Ui::SearchWidget())
    , m_searchWidget(new QWidget())
    , m_newGroup(Q_NULLPTR)
    , m_newEntry(Q_NULLPTR)
    , m_newParent(Q_NULLPTR)
    , m_menuGroup(new QMenu(this))
    , m_menuEntry(new QMenu(this))
{
    m_searchUi->setupUi(m_searchWidget);

    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);

    m_mainWidget = new QWidget(this);
    QLayout* layout = new QHBoxLayout(m_mainWidget);
    QSplitter* splitter = new QSplitter(m_mainWidget);

    QWidget* rightHandSideWidget = new QWidget(splitter);
    m_searchWidget->setParent(rightHandSideWidget);

    m_groupView = new GroupView(db, splitter);
    m_groupView->setObjectName("groupView");
    m_groupView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_groupView, SIGNAL(customContextMenuRequested(QPoint)), SLOT(showGroupContextMenu(QPoint)));

    m_entryView = new EntryView(rightHandSideWidget);
    m_entryView->setObjectName("entryView");
    m_entryView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_entryView->setGroup(db->rootGroup());
    connect(m_entryView, SIGNAL(customContextMenuRequested(QPoint)), SLOT(showEntryContextMenu(QPoint)));

    QSizePolicy policy;
    policy = m_groupView->sizePolicy();
    policy.setHorizontalStretch(30);
    m_groupView->setSizePolicy(policy);
    policy = rightHandSideWidget->sizePolicy();
    policy.setHorizontalStretch(70);
    rightHandSideWidget->setSizePolicy(policy);

    QAction* closeAction = new QAction(m_searchWidget);
    QIcon closeIcon = filePath()->icon("actions", "dialog-close");
    closeAction->setIcon(closeIcon);
    m_searchUi->closeSearchButton->setDefaultAction(closeAction);
    m_searchUi->closeSearchButton->setShortcut(Qt::Key_Escape);
    m_searchWidget->hide();
    m_searchUi->caseSensitiveCheckBox->setVisible(false);

    QVBoxLayout* vLayout = new QVBoxLayout(rightHandSideWidget);
    vLayout->setMargin(0);
    vLayout->addWidget(m_searchWidget);
    vLayout->addWidget(m_entryView);

    rightHandSideWidget->setLayout(vLayout);

    splitter->addWidget(m_groupView);
    splitter->addWidget(rightHandSideWidget);

    layout->addWidget(splitter);
    m_mainWidget->setLayout(layout);

    m_editEntryWidget = new EditEntryWidget();
    m_editEntryWidget->setObjectName("editEntryWidget");
    m_historyEditEntryWidget = new EditEntryWidget();
    m_editGroupWidget = new EditGroupWidget();
    m_editGroupWidget->setObjectName("editGroupWidget");
    m_changeMasterKeyWidget = new ChangeMasterKeyWidget();
    m_changeMasterKeyWidget->headlineLabel()->setText(tr("Change master key"));
    QFont headlineLabelFont = m_changeMasterKeyWidget->headlineLabel()->font();
    headlineLabelFont.setBold(true);
    headlineLabelFont.setPointSize(headlineLabelFont.pointSize() + 2);
    m_changeMasterKeyWidget->headlineLabel()->setFont(headlineLabelFont);
    m_databaseSettingsWidget = new DatabaseSettingsWidget();
    m_databaseSettingsWidget->setObjectName("databaseSettingsWidget");
    m_databaseOpenWidget = new DatabaseOpenWidget();
    m_databaseOpenWidget->setObjectName("databaseOpenWidget");
    m_keepass1OpenWidget = new KeePass1OpenWidget();
    m_keepass1OpenWidget->setObjectName("keepass1OpenWidget");
    addWidget(m_mainWidget);
    addWidget(m_editEntryWidget);
    addWidget(m_editGroupWidget);
    addWidget(m_changeMasterKeyWidget);
    addWidget(m_databaseSettingsWidget);
    addWidget(m_historyEditEntryWidget);
    addWidget(m_databaseOpenWidget);
    addWidget(m_keepass1OpenWidget);

    m_actionEntryNew = m_menuEntry->addAction(tr("Add new entry"), this, SLOT(createEntry()));
    m_actionEntryNew->setIcon(filePath()->icon("actions", "entry-new", false));
    m_actionEntryClone = m_menuEntry->addAction(tr("Clone entry"), this, SLOT(cloneEntry()));
    m_actionEntryClone->setIcon(filePath()->icon("actions", "entry-clone", false));
    m_actionEntryClone->setEnabled(false);
    m_actionEntryEditView = m_menuEntry->addAction(tr("View/Edit entry"), this, SLOT(switchToEntryEdit()));
    m_actionEntryEditView->setIcon(filePath()->icon("actions", "entry-edit", false));
    m_actionEntryEditView->setEnabled(false);
    m_actionEntryDelete = m_menuEntry->addAction(tr("Delete entry"), this, SLOT(deleteEntry()));
    m_actionEntryDelete->setIcon(filePath()->icon("actions", "entry-delete", false));
    m_actionEntryDelete->setEnabled(false);
    m_actionEntryCopyUsername = m_menuEntry->addAction(tr("Copy username to clipboard"), this,
                                                       SLOT(copyUsername()), Qt::CTRL + Qt::Key_B);
    m_actionEntryCopyUsername->setEnabled(false);
    m_actionEntryCopyPassword = m_menuEntry->addAction(tr("Copy password to clipboard"), this,
                                                       SLOT(copyPassword()), Qt::CTRL + Qt::Key_C);
    m_actionEntryCopyPassword->setEnabled(false);
    m_actionEntryAutoType = m_menuEntry->addAction(tr("Perform Auto-Type"), this,
                                                   SLOT(performAutoType()));
    if (!QKeySequence::keyBindings(QKeySequence::Paste).isEmpty()) {
        m_actionEntryAutoType->setShortcuts(QKeySequence::Paste);
    }
    else {
        m_actionEntryAutoType->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_V));
    }
    m_actionEntryAutoType->setEnabled(false);

    m_actionGroupNew = m_menuGroup->addAction(tr("Add new group"), this, SLOT(createGroup()));
    m_actionGroupNew->setIcon(filePath()->icon("actions", "group-new", false));
    m_actionGroupEdit = m_menuGroup->addAction(tr("Edit group"), this, SLOT(switchToGroupEdit()));
    m_actionGroupEdit->setIcon(filePath()->icon("actions", "group-edit", false));
    m_actionGroupDelete = m_menuGroup->addAction(tr("Delete group"), this, SLOT(deleteGroup()));
    m_actionGroupDelete->setIcon(filePath()->icon("actions", "group-delete", false));
    m_actionGroupDelete->setEnabled(false);

    connect(m_groupView, SIGNAL(groupChanged(Group*)), this, SLOT(clearLastGroup(Group*)));
    connect(m_groupView, SIGNAL(groupChanged(Group*)), SLOT(updateGroupActions(Group*)));
    connect(m_groupView, SIGNAL(groupChanged(Group*)), m_entryView, SLOT(setGroup(Group*)));
    connect(m_entryView, SIGNAL(entryActivated(Entry*)), SLOT(switchToEntryEdit(Entry*)));
    connect(m_editEntryWidget, SIGNAL(editFinished(bool)), SLOT(switchToView(bool)));
    connect(m_editEntryWidget, SIGNAL(historyEntryActivated(Entry*)), SLOT(switchToHistoryView(Entry*)));
    connect(m_historyEditEntryWidget, SIGNAL(editFinished(bool)), SLOT(switchBackToEntryEdit()));
    connect(m_editGroupWidget, SIGNAL(editFinished(bool)), SLOT(switchToView(bool)));
    connect(m_changeMasterKeyWidget, SIGNAL(editFinished(bool)), SLOT(updateMasterKey(bool)));
    connect(m_databaseSettingsWidget, SIGNAL(editFinished(bool)), SLOT(switchToView(bool)));
    connect(m_databaseOpenWidget, SIGNAL(editFinished(bool)), SLOT(openDatabase(bool)));
    connect(m_keepass1OpenWidget, SIGNAL(editFinished(bool)), SLOT(openDatabase(bool)));
    connect(this, SIGNAL(currentChanged(int)), this, SLOT(emitCurrentModeChanged()));
    connect(m_searchUi->searchEdit, SIGNAL(textChanged(QString)), this, SLOT(startSearchTimer()));
    connect(m_searchUi->caseSensitiveCheckBox, SIGNAL(toggled(bool)), this, SLOT(startSearch()));
    connect(m_searchUi->searchCurrentRadioButton, SIGNAL(toggled(bool)), this, SLOT(startSearch()));
    connect(m_searchUi->searchRootRadioButton, SIGNAL(toggled(bool)), this, SLOT(startSearch()));
    connect(m_searchTimer, SIGNAL(timeout()), this, SLOT(search()));
    connect(closeAction, SIGNAL(triggered()), this, SLOT(closeSearch()));
    connect(m_entryView, SIGNAL(entrySelectionChanged()), SLOT(updateEntryActions()));

    setCurrentIndex(0);
}

DatabaseWidget::~DatabaseWidget()
{
}

DatabaseWidget::Mode DatabaseWidget::currentMode()
{
    switch (currentIndex()) {
    case -1:
        return DatabaseWidget::None;
    case 0:
        return DatabaseWidget::ViewMode;
    case 1:  // entry edit
    case 2:  // group edit
    case 3:  // change master key
    case 4:  // database settings
    case 5:  // entry history
    case 6:  // open database
    case 7:  // keepass 1 import
        return DatabaseWidget::EditMode;
    default:
        Q_ASSERT(false);
        return DatabaseWidget::None;
    }
}

bool DatabaseWidget::actionEnabled(Action action)
{
    switch (action) {
    case GroupNew:
        return m_actionGroupNew->isEnabled();
    case GroupEdit:
        return m_actionGroupEdit->isEnabled();
    case GroupDelete:
        return m_actionGroupDelete->isEnabled();
    case EntryNew:
        return m_actionEntryNew->isEnabled();
    case EntryClone:
        return m_actionEntryClone->isEnabled();
    case EntryEditView:
        return m_actionEntryEditView->isEnabled();
    case EntryDelete:
        return m_actionEntryDelete->isEnabled();
    case EntryCopyUsername:
        return m_actionEntryCopyUsername->isEnabled();
    case EntryCopyPassword:
        return m_actionEntryCopyPassword->isEnabled();
    case EntryAutoType:
        return m_actionEntryAutoType->isEnabled();
    default:
        Q_ASSERT(false);
        return false;
    }
}

void DatabaseWidget::emitCurrentModeChanged()
{
    Q_EMIT currentModeChanged(currentMode());
}

GroupView* DatabaseWidget::groupView()
{
    return m_groupView;
}

EntryView* DatabaseWidget::entryView()
{
    return m_entryView;
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
    switchToEntryEdit(m_newEntry, true);
}

void DatabaseWidget::cloneEntry()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return;
    }

    Entry* entry = currentEntry->clone();
    entry->setUuid(Uuid::random());
    entry->setGroup(currentEntry->group());
    m_entryView->setFocus();
    m_entryView->setCurrentEntry(entry);
}

void DatabaseWidget::deleteEntry()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return;
    }

    bool inRecylceBin = Tools::hasChild(m_db->metadata()->recycleBin(), currentEntry);
    if (inRecylceBin || !m_db->metadata()->recycleBinEnabled()) {
        QMessageBox::StandardButton result = QMessageBox::question(
            this, tr("Delete entry?"),
            tr("Do you really want to delete the entry \"%1\" for good?")
            .arg(currentEntry->title()),
            QMessageBox::Yes | QMessageBox::No);
        if (result == QMessageBox::Yes) {
            delete currentEntry;
        }
    }
    else {
        m_db->recycleEntry(currentEntry);
    }
}

void DatabaseWidget::copyUsername()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return;
    }

    clipboard()->setText(currentEntry->username());
}

void DatabaseWidget::copyPassword()
{
    Entry* currentEntry = m_entryView->currentEntry();
    if (!currentEntry) {
        Q_ASSERT(false);
        return;
    }

    clipboard()->setText(currentEntry->password());
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
    if (!currentGroup || !canDeleteCurrentGoup()) {
        Q_ASSERT(false);
        return;
    }

    bool inRecylceBin = Tools::hasChild(m_db->metadata()->recycleBin(), currentGroup);
    if (inRecylceBin || !m_db->metadata()->recycleBinEnabled()) {
        QMessageBox::StandardButton result = QMessageBox::question(
            this, tr("Delete group?"),
            tr("Do you really want to delete the group \"%1\" for good?")
            .arg(currentGroup->name()),
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
    if (currentWidget()) {
        currentWidget()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    }

    QStackedWidget::setCurrentIndex(index);

    if (currentWidget()) {
        currentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }

    adjustSize();
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

        m_newGroup = Q_NULLPTR;
        m_newParent = Q_NULLPTR;
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

        m_newEntry = Q_NULLPTR;
        m_newParent = Q_NULLPTR;
    }

    setCurrentIndex(0);
}

void DatabaseWidget::switchToHistoryView(Entry* entry)
{
    m_historyEditEntryWidget->loadEntry(entry, false, true, "", m_db);
    setCurrentIndex(5);
}

void DatabaseWidget::switchBackToEntryEdit()
{
    setCurrentIndex(1);
}

void DatabaseWidget::switchToEntryEdit(Entry* entry)
{
    switchToEntryEdit(entry, false);
}

void DatabaseWidget::switchToEntryEdit(Entry* entry, bool create)
{
    Group* group = m_groupView->currentGroup();
    if (!group) {
        Q_ASSERT(m_entryView->inEntryListMode());
        group = m_lastGroup;
    }
    Q_ASSERT(group);

    m_editEntryWidget->loadEntry(entry, create, false, group->name(), m_db);
    setCurrentIndex(1);
}

void DatabaseWidget::switchToGroupEdit(Group* group, bool create)
{
    m_editGroupWidget->loadGroup(group, create, m_db);
    setCurrentIndex(2);
}

void DatabaseWidget::updateMasterKey(bool accepted)
{
    if (accepted) {
        m_db->setKey(m_changeMasterKeyWidget->newMasterKey());
    }
    else if (!m_db->hasKey()) {
        Q_EMIT closeRequest();
        return;
    }

    setCurrentIndex(0);
}

void DatabaseWidget::openDatabase(bool accepted)
{
    if (accepted) {
        Database* oldDb = m_db;
        m_db = static_cast<DatabaseOpenWidget*>(sender())->database();
        m_groupView->changeDatabase(m_db);
        Q_EMIT databaseChanged(m_db);
        delete oldDb;
        setCurrentIndex(0);

        // We won't need those anymore and KeePass1OpenWidget closes
        // the file in its dtor.
        delete m_databaseOpenWidget;
        m_databaseOpenWidget = Q_NULLPTR;
        delete m_keepass1OpenWidget;
        m_keepass1OpenWidget = Q_NULLPTR;
    }
    else {
        if (m_databaseOpenWidget->database()) {
            delete m_databaseOpenWidget->database();
        }
        Q_EMIT closeRequest();
    }
}

void DatabaseWidget::switchToEntryEdit()
{
    switchToEntryEdit(m_entryView->currentEntry(), false);
}

void DatabaseWidget::switchToGroupEdit()
{
    switchToGroupEdit(m_groupView->currentGroup(), false);
}

void DatabaseWidget::switchToMasterKeyChange()
{
    m_changeMasterKeyWidget->clearForms();
    setCurrentIndex(3);
}

void DatabaseWidget::switchToDatabaseSettings()
{
    m_databaseSettingsWidget->load(m_db);
    setCurrentIndex(4);
}

void DatabaseWidget::switchToOpenDatabase(const QString& fileName)
{
    m_databaseOpenWidget->load(fileName);
    setCurrentIndex(6);
}

void DatabaseWidget::switchToOpenDatabase(const QString& fileName, const QString& password,
                                          const QString& keyFile)
{
    switchToOpenDatabase(fileName);
    m_databaseOpenWidget->enterKey(password, keyFile);
}

void DatabaseWidget::switchToImportKeepass1(const QString& fileName)
{
    m_keepass1OpenWidget->load(fileName);
    setCurrentIndex(7);
}

void DatabaseWidget::toggleSearch()
{
    if (m_entryView->inEntryListMode()) {
        closeSearch();
    }
    else {
        showSearch();
    }
}

void DatabaseWidget::closeSearch()
{
    Q_ASSERT(m_lastGroup);
    m_groupView->setCurrentGroup(m_lastGroup);
}

void DatabaseWidget::showSearch()
{
    m_searchUi->searchEdit->blockSignals(true);
    m_searchUi->searchEdit->clear();
    m_searchUi->searchEdit->blockSignals(false);

    m_searchUi->searchCurrentRadioButton->blockSignals(true);
    m_searchUi->searchRootRadioButton->blockSignals(true);
    m_searchUi->searchCurrentRadioButton->setChecked(true);
    m_searchUi->searchCurrentRadioButton->blockSignals(false);
    m_searchUi->searchRootRadioButton->blockSignals(false);

    m_lastGroup = m_groupView->currentGroup();

    Q_ASSERT(m_lastGroup);

    if (m_lastGroup == m_db->rootGroup()) {
        m_searchUi->optionsWidget->hide();
        m_searchUi->searchCurrentRadioButton->hide();
        m_searchUi->searchRootRadioButton->hide();
    }
    else {
        m_searchUi->optionsWidget->show();
        m_searchUi->searchCurrentRadioButton->show();
        m_searchUi->searchRootRadioButton->show();
        m_searchUi->searchCurrentRadioButton->setText(tr("Current group")
                                                      .append(" (")
                                                      .append(m_lastGroup->name())
                                                      .append(")"));
    }
    m_groupView->setCurrentIndex(QModelIndex());

    m_searchWidget->show();
    search();
    m_searchUi->searchEdit->setFocus();
}

void DatabaseWidget::search()
{
    Q_ASSERT(m_lastGroup);

    Group* searchGroup;
    if (m_searchUi->searchCurrentRadioButton->isChecked()) {
        searchGroup = m_lastGroup;
    }
    else if (m_searchUi->searchRootRadioButton->isChecked()) {
        searchGroup = m_db->rootGroup();
    }
    else {
        Q_ASSERT(false);
        return;
    }

    Qt::CaseSensitivity sensitivity;
    if (m_searchUi->caseSensitiveCheckBox->isChecked()) {
        sensitivity = Qt::CaseSensitive;
    }
    else {
        sensitivity = Qt::CaseInsensitive;
    }
    QList<Entry*> searchResult = searchGroup->search(m_searchUi->searchEdit->text(), sensitivity);


    m_entryView->setEntryList(searchResult);
}

void DatabaseWidget::startSearchTimer()
{
    if (!m_searchTimer->isActive()) {
        m_searchTimer->stop();
    }
    m_searchTimer->start(100);
}

void DatabaseWidget::startSearch()
{
    if (!m_searchTimer->isActive()) {
        m_searchTimer->stop();
    }
    search();
}

bool DatabaseWidget::dbHasKey()
{
    return m_db->hasKey();
}

bool DatabaseWidget::canDeleteCurrentGoup()
{
    bool isRootGroup = m_db->rootGroup() == m_groupView->currentGroup();
    bool isRecycleBin = m_db->metadata()->recycleBin() == m_groupView->currentGroup();
    return !isRootGroup && !isRecycleBin;
}

void DatabaseWidget::clearLastGroup(Group* group)
{
    if (group) {
        m_lastGroup = Q_NULLPTR;
        m_searchWidget->hide();
    }
}

void DatabaseWidget::updateGroupActions(Group* group)
{
    m_actionGroupNew->setEnabled(group);
    m_actionGroupEdit->setEnabled(group);
    m_actionGroupDelete->setEnabled(group && canDeleteCurrentGoup());
}

void DatabaseWidget::updateEntryActions()
{
    bool singleEntrySelected = m_entryView->isSingleEntrySelected();
    bool inSearch = m_entryView->inEntryListMode();

    m_actionEntryNew->setEnabled(!inSearch);
    m_actionEntryClone->setEnabled(singleEntrySelected && !inSearch);
    m_actionEntryEditView->setEnabled(singleEntrySelected);
    m_actionEntryDelete->setEnabled(singleEntrySelected);
    m_actionEntryCopyUsername->setEnabled(singleEntrySelected);
    m_actionEntryCopyPassword->setEnabled(singleEntrySelected);
    m_actionEntryAutoType->setEnabled(singleEntrySelected);
}

void DatabaseWidget::showGroupContextMenu(const QPoint& pos)
{
    m_menuGroup->popup(m_groupView->viewport()->mapToGlobal(pos));
}

void DatabaseWidget::showEntryContextMenu(const QPoint& pos)
{
    m_menuEntry->popup(m_entryView->viewport()->mapToGlobal(pos));
}
