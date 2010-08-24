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

#include "EntryView.h"
#include "GroupView.h"

DatabaseWidget::DatabaseWidget(Database* db, QWidget* parent)
    : QWidget(parent)
{
    m_groupView = new GroupView(db);
    m_entryView = new EntryView();

    connect(m_groupView, SIGNAL(groupChanged(Group*)), m_entryView, SLOT(setGroup(Group*)));

    QHBoxLayout* layout = new QHBoxLayout();
    layout->addWidget(m_groupView);
    layout->addWidget(m_entryView);
    setLayout(layout);
}
