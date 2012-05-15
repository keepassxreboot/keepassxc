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

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QSplitter>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>

#include "core/Metadata.h"
#include "core/Tools.h"
#include "gui/ChangeMasterKeyWidget.h"
#include "gui/DatabaseSettingsWidget.h"
#include "gui/EditEntryWidget.h"
#include "gui/EditGroupWidget.h"
#include "gui/EntryView.h"
#include "gui/GroupView.h"

DatabaseWidget::DatabaseWidget(Database* db, QWidget* parent)
    : QStackedWidget(parent)
    , m_db(db)
    , m_newGroup(0)
    , m_newEntry(0)
    , m_newParent(0)
{
    m_mainWidget = new QWidget(this);
    QLayout* layout = new QHBoxLayout(m_mainWidget);
    QSplitter* splitter = new QSplitter(m_mainWidget);

    m_groupView = new GroupView(db, splitter);
    m_groupView->setObjectName("groupView");
    m_entryView = new EntryView(splitter);
    m_entryView->setObjectName("entryView");

    QSizePolicy policy;

    policy = m_groupView->sizePolicy();
    policy.setHorizontalStretch(30);
    m_groupView->setSizePolicy(policy);

    QWidget* widget = new QWidget();
    policy = widget->sizePolicy();
    policy.setHorizontalStretch(70);
    widget->setSizePolicy(policy);

    splitter->addWidget(m_groupView);

    QVBoxLayout* vLayout = new QVBoxLayout();
    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->addWidget(new QLabel("Find:"));
    m_searchEdit = new QLineEdit();
    hLayout->addWidget(m_searchEdit);
    vLayout->addLayout(hLayout);
    vLayout->addWidget(m_entryView);
    widget->setLayout(vLayout);
    splitter->addWidget(widget);

    layout->addWidget(splitter);
    m_mainWidget->setLayout(layout);

    m_editEntryWidget = new EditEntryWidget();
    m_editEntryWidget->setObjectName("editEntryWidget");
    m_editGroupWidget = new EditGroupWidget();
    m_editGroupWidget->setObjectName("editGroupWidget");
    m_changeMasterKeyWidget = new ChangeMasterKeyWidget();
    m_changeMasterKeyWidget->headlineLabel()->setText(tr("Change master key"));
    QFont headlineLabelFont = m_changeMasterKeyWidget->headlineLabel()->font();
    headlineLabelFont.setBold(true);
    headlineLabelFont.setPointSize(headlineLabelFont.pointSize() + 2);
    m_changeMasterKeyWidget->headlineLabel()->setFont(headlineLabelFont);
    m_databaseSettingsWidget = new DatabaseSettingsWidget();
    addWidget(m_mainWidget);
    addWidget(m_editEntryWidget);
    addWidget(m_editGroupWidget);
    addWidget(m_changeMasterKeyWidget);
    addWidget(m_databaseSettingsWidget);

    connect(m_groupView, SIGNAL(groupChanged(Group*)), m_entryView, SLOT(setGroup(Group*)));
    connect(m_groupView, SIGNAL(groupChanged(Group*)), this, SLOT(clearLastGroup(Group*)));
    connect(m_entryView, SIGNAL(entryActivated(Entry*)), SLOT(switchToEntryEdit(Entry*)));
    connect(m_editEntryWidget, SIGNAL(editFinished(bool)), SLOT(switchToView(bool)));
    connect(m_editGroupWidget, SIGNAL(editFinished(bool)), SLOT(switchToView(bool)));
    connect(m_changeMasterKeyWidget, SIGNAL(editFinished(bool)), SLOT(updateMasterKey(bool)));
    connect(m_databaseSettingsWidget, SIGNAL(editFinished(bool)), SLOT(updateSettings(bool)));
    connect(this, SIGNAL(currentChanged(int)), this, SLOT(emitCurrentModeChanged()));
    connect(m_searchEdit, SIGNAL(returnPressed()), this, SLOT(search()));

    setCurrentIndex(0);
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
        return DatabaseWidget::EditMode;
    default:
        Q_ASSERT(false);
        return DatabaseWidget::None;
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

void DatabaseWidget::createEntry()
{
    m_newEntry = new Entry();
    m_newEntry->setUuid(Uuid::random());
    m_newEntry->setUsername(m_db->metadata()->defaultUserName());
    m_newParent = m_groupView->currentGroup();
    switchToEntryEdit(m_newEntry, true);
}

void DatabaseWidget::deleteEntry()
{
    bool inRecylceBin = Tools::hasChild(m_db->metadata()->recycleBin(), m_entryView->currentEntry());
    if (inRecylceBin || !m_db->metadata()->recycleBinEnabled()) {
        QMessageBox::StandardButton result = QMessageBox::question(
            this, tr("Delete entry?"),
            tr("Do you really want to delete the entry \"%1\" for good?")
            .arg(m_entryView->currentEntry()->title()),
            QMessageBox::Yes | QMessageBox::No);
        if (result == QMessageBox::Yes) {
            delete m_entryView->currentEntry();
        }
    }
    else {
        m_db->recycleEntry(m_entryView->currentEntry());
    }
}

void DatabaseWidget::createGroup()
{
    m_newGroup = new Group();
    m_newGroup->setUuid(Uuid::random());
    m_newParent = m_groupView->currentGroup();
    switchToGroupEdit(m_newGroup, true);
}

void DatabaseWidget::deleteGroup()
{
    Q_ASSERT(canDeleteCurrentGoup());

    bool inRecylceBin = Tools::hasChild(m_db->metadata()->recycleBin(), m_groupView->currentGroup());
    if (inRecylceBin || !m_db->metadata()->recycleBinEnabled()) {
        QMessageBox::StandardButton result = QMessageBox::question(
            this, tr("Delete group?"),
            tr("Do you really want to delete the group \"%1\" for good?")
            .arg(m_groupView->currentGroup()->name()),
            QMessageBox::Yes | QMessageBox::No);
        if (result == QMessageBox::Yes) {
            delete m_groupView->currentGroup();
        }
    }
    else {
        m_db->recycleGroup(m_groupView->currentGroup());
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

        m_newGroup = 0;
        m_newParent = 0;
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

        m_newEntry = 0;
        m_newParent = 0;
    }

    setCurrentIndex(0);
}

void DatabaseWidget::switchToEntryEdit(Entry* entry)
{
    switchToEntryEdit(entry, false);
}

void DatabaseWidget::switchToEntryEdit(Entry* entry, bool create)
{
    Group* group = m_groupView->currentGroup();
    if (!group) {
        Q_ASSERT(m_entryView->inSearch());
        group = m_lastGroup;
    }
    Q_ASSERT(group);

    m_editEntryWidget->loadEntry(entry, create, group->name(), m_db);
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

void DatabaseWidget::updateSettings(bool accepted)
{
    if (accepted) {
        m_db->updateKey(m_databaseSettingsWidget->transformRounds());
        m_db->metadata()->setDescription(m_databaseSettingsWidget->dbDescription());
        m_db->metadata()->setDefaultUserName(m_databaseSettingsWidget->defaultUsername());
        m_db->metadata()->setRecycleBinEnabled(m_databaseSettingsWidget->recylceBinEnabled());
        m_db->metadata()->setName(m_databaseSettingsWidget->dbName());
        if (m_db->metadata()->historyMaxItems() != m_databaseSettingsWidget->historyMaxItems() ||
                m_db->metadata()->historyMaxSize() != m_databaseSettingsWidget->historyMaxSize()) {
            m_db->metadata()->setHistoryMaxItems(m_databaseSettingsWidget->historyMaxItems());
            m_db->metadata()->setHistoryMaxSize(m_databaseSettingsWidget->historyMaxSize());
            truncateHistories();
        }
    }

    setCurrentIndex(0);
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
    m_databaseSettingsWidget->setForms(m_db->metadata()->name(),
                                       m_db->metadata()->description(),
                                       m_db->metadata()->defaultUserName(),
                                       m_db->metadata()->recycleBinEnabled(),
                                       m_db->transformRounds(),
                                       m_db->metadata()->historyMaxItems(),
                                       m_db->metadata()->historyMaxSize());
    setCurrentIndex(4);
}

void DatabaseWidget::search()
{
    Group* searchGroup = m_db->rootGroup();
    QList<Entry*> searchResult = searchGroup->search(m_searchEdit->text(), Qt::CaseInsensitive);

    Group* group = m_groupView->currentGroup();
    if (group) {
        m_lastGroup = m_groupView->currentGroup();
    }

    m_groupView->setCurrentIndex(QModelIndex());
    m_entryView->search(searchResult);
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

void DatabaseWidget::truncateHistories()
{
    QList<Entry*> allEntries = m_db->rootGroup()->entriesRecursive(false);
    Q_FOREACH (Entry* entry, allEntries) {
        entry->truncateHistory();
    }
}

void DatabaseWidget::clearLastGroup(Group* group)
{
    if (group) {
        m_lastGroup = 0;
    }
}
