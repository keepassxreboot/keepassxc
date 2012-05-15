/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#include "EditGroupWidget.h"
#include "ui_EditGroupWidgetMain.h"
#include "ui_EditWidget.h"

#include "gui/EditWidgetIcons.h"

EditGroupWidget::EditGroupWidget(QWidget* parent)
    : EditWidget(parent)
    , m_mainUi(new Ui::EditGroupWidgetMain())
    , m_editGroupWidgetMain(new QWidget())
    , m_editGroupWidgetIcons(new EditWidgetIcons())
    , m_group(0)
{
    m_mainUi->setupUi(m_editGroupWidgetMain);

    add(tr("Group"), m_editGroupWidgetMain);
    add(tr("Icon"), m_editGroupWidgetIcons);

    QFont labelHeaderFont = headlineLabel()->font();
    labelHeaderFont.setBold(true);
    labelHeaderFont.setPointSize(labelHeaderFont.pointSize() + 2);
    headlineLabel()->setFont(labelHeaderFont);

    connect(this, SIGNAL(accepted()), SLOT(save()));
    connect(this, SIGNAL(rejected()), SLOT(cancel()));
}

EditGroupWidget::~EditGroupWidget()
{
}

void EditGroupWidget::loadGroup(Group* group, bool create, Database* database)
{
    m_group = group;

    if (create) {
        headlineLabel()->setText(tr("Add group"));
    }
    else {
        headlineLabel()->setText(tr("Edit group"));
    }

    m_mainUi->editName->setText(m_group->name());
    m_mainUi->editNotes->setPlainText(m_group->notes());

    setCurrentRow(0);
    IconStruct iconStruct;
    iconStruct.uuid = group->iconUuid();
    iconStruct.number = group->iconNumber();
    m_editGroupWidgetIcons->load(group->uuid(), database, iconStruct);

    m_mainUi->editName->setFocus();
}

void EditGroupWidget::save()
{
    m_group->setName(m_mainUi->editName->text());
    m_group->setNotes(m_mainUi->editNotes->toPlainText());

    IconStruct iconStruct = m_editGroupWidgetIcons->save();

    if (iconStruct.uuid.isNull()) {
        m_group->setIcon(iconStruct.number);
    }
    else {
        m_group->setIcon(iconStruct.uuid);
    }

    m_group = 0;
    Q_EMIT editFinished(true);
}

void EditGroupWidget::cancel()
{
    m_group = 0;
    Q_EMIT editFinished(false);
}
