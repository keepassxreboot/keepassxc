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
#include "ui_EditGroupWidget.h"

EditGroupWidget::EditGroupWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::EditGroupWidget())
    , m_group(0)
{
    m_ui->setupUi(this);

    QFont labelHeaderFont = m_ui->labelHeader->font();
    labelHeaderFont.setBold(true);
    labelHeaderFont.setPointSize(labelHeaderFont.pointSize() + 2);
    m_ui->labelHeader->setFont(labelHeaderFont);

    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(save()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(cancel()));
}

EditGroupWidget::~EditGroupWidget()
{
}

void EditGroupWidget::loadGroup(Group* group, bool create)
{
    m_group = group;

    if (create) {
        m_ui->labelHeader->setText(tr("Add group"));
    }
    else {
        m_ui->labelHeader->setText(tr("Edit group"));
    }

    m_ui->editName->setText(m_group->name());
    m_ui->editNotes->setPlainText(m_group->notes());
}

void EditGroupWidget::save()
{
    m_group->setName(m_ui->editName->text());
    m_group->setNotes(m_ui->editNotes->toPlainText());

    m_group = 0;
    Q_EMIT editFinished(true);
}

void EditGroupWidget::cancel()
{
    m_group = 0;
    Q_EMIT editFinished(false);
}
