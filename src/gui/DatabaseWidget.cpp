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
#include <QtGui/QSplitter>

#include "gui/EditEntryWidget.h"
#include "gui/EditGroupWidget.h"
#include "gui/EntryView.h"
#include "gui/GroupView.h"

DatabaseWidget::DatabaseWidget(Database* db, QWidget* parent)
    : QStackedWidget(parent)
    , m_newGroup(0)
    , m_newParent(0)
{
    m_mainWidget = new QWidget(this);
    QLayout* layout = new QHBoxLayout(m_mainWidget);
    QSplitter* splitter = new QSplitter(m_mainWidget);

    m_groupView = new GroupView(db, splitter);
    m_entryView = new EntryView(splitter);

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
    m_editGroupWidget = new EditGroupWidget();

    addWidget(m_mainWidget);
    addWidget(m_editEntryWidget);
    addWidget(m_editGroupWidget);

    connect(m_groupView, SIGNAL(groupChanged(Group*)), m_entryView, SLOT(setGroup(Group*)));
    connect(m_entryView, SIGNAL(entryActivated(Entry*)), SLOT(switchToEntryEdit(Entry*)));
    connect(m_editEntryWidget, SIGNAL(editFinished(bool)), SLOT(switchToView(bool)));
    connect(m_editGroupWidget, SIGNAL(editFinished(bool)), SLOT(switchToView(bool)));

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
void DatabaseWidget::switchToEntryEdit()
{
    switchToEntryEdit(m_entryView->currentEntry(), false);
}

void DatabaseWidget::switchToGroupEdit()
{
    switchToGroupEdit(m_groupView->currentGroup(), false);
}
