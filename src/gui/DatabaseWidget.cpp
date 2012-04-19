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

    policy = m_entryView->sizePolicy();
    policy.setHorizontalStretch(70);
    m_entryView->setSizePolicy(policy);

    splitter->addWidget(m_groupView);
    splitter->addWidget(m_entryView);

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
    connect(m_entryView, SIGNAL(entryActivated(Entry*)), SLOT(switchToEntryEdit(Entry*)));
    connect(m_editEntryWidget, SIGNAL(editFinished(bool)), SLOT(switchToView(bool)));
    connect(m_editGroupWidget, SIGNAL(editFinished(bool)), SLOT(switchToView(bool)));
    connect(m_changeMasterKeyWidget, SIGNAL(editFinished(bool)), SLOT(updateMasterKey(bool)));
    connect(m_databaseSettingsWidget, SIGNAL(editFinished(bool)), SLOT(updateSettings(bool)));

    setCurrentIndex(0);
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
    m_newParent = m_groupView->currentGroup();
    switchToEntryEdit(m_newEntry, true);
}

void DatabaseWidget::deleteEntry()
{
    bool inRecylceBin = Tools::hasChild(m_db->metadata()->recycleBin(), m_entryView->currentEntry());
    if (inRecylceBin || !m_db->metadata()->recycleBinEnabled()) {
        QMessageBox::StandardButton result = QMessageBox::question(
            this, tr("Question"), tr("Do you really want to delete this entry for good?"),
            QMessageBox::Yes | QMessageBox::No);
        if (result == QMessageBox::Yes) {
            delete m_entryView->currentEntry();
        }
        else {
            return;
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

void DatabaseWidget::switchToView(bool accepted)
{
    if (m_newGroup) {
        if (accepted) {
            m_newGroup->setParent(m_newParent);
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
    m_editEntryWidget->loadEntry(entry, create, m_groupView->currentGroup()->name());
    setCurrentIndex(1);
}

void DatabaseWidget::switchToGroupEdit(Group* group, bool create)
{
    m_editGroupWidget->loadGroup(group, create);
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
        m_db->metadata()->setRecycleBinEnabled(m_databaseSettingsWidget->recylceBinEnabled());
        m_db->metadata()->setName(m_databaseSettingsWidget->dbName());
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
                                       m_db->metadata()->recycleBinEnabled(),
                                       m_db->transformRounds());
    setCurrentIndex(4);
}

bool DatabaseWidget::dbHasKey()
{
    return m_db->hasKey();
}
