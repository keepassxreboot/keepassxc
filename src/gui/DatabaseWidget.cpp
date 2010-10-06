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

#include "EditEntryWidget.h"
#include "EntryView.h"
#include "GroupView.h"

DatabaseWidget::DatabaseWidget(Database* db, QWidget* parent)
    : QStackedWidget(parent)
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

    m_editWidget = new EditEntryWidget();

    addWidget(m_mainWidget);
    addWidget(m_editWidget);

    connect(m_groupView, SIGNAL(groupChanged(Group*)), m_entryView, SLOT(setGroup(Group*)));
    connect(m_entryView, SIGNAL(entryActivated(Entry*)), SLOT(switchToEdit(Entry*)));
    connect(m_editWidget, SIGNAL(editFinished()), SLOT(switchToView()));

    setCurrentIndex(0);
}

void DatabaseWidget::switchToView()
{
    setCurrentIndex(0);
}

void DatabaseWidget::switchToEdit(Entry* entry)
{
    m_editWidget->loadEntry(entry);
    setCurrentIndex(1);
}
