/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#include "autotype/AutoType.h"
#include "gui/AutoTypeTargetSelectDialog.h"
#include "gui/MessageBox.h"
#include "ui_AutoTypeTargetSelectDialog.h"
#include <utility>

AutoTypeTargetSelectDialog::AutoTypeTargetSelectDialog(QString pluginName,
                                                       const AutoTypeTargetMap& targetList,
                                                       QWidget* parent,
                                                       Entry* entry)
    : QDialog(parent)
    , m_ui(new Ui::AutoTypeTargetSelectDialog())
    , m_entry(entry)
    , m_pluginName(std::move(pluginName))
    , m_targetList(targetList)
{
    m_ui->setupUi(this);
    this->setFixedHeight(this->sizeHint().height());

    setAttribute(Qt::WA_DeleteOnClose);

    for (const QSharedPointer<AutoTypeTarget>& target : m_targetList.values()) {
        m_ui->targetComboBox->addItem(target->getPresentableName(), target->getIdentifier());
    }

    m_ui->targetComboBox->setAutoCompletion(true);

    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(performAutoType()));
}

void AutoTypeTargetSelectDialog::performAutoType()
{
    if (m_ui->targetComboBox->itemText(m_ui->targetComboBox->currentIndex()) != m_ui->targetComboBox->currentText() ||
        m_ui->targetComboBox->currentData().toString().isEmpty()) {
        MessageBox::critical(this, "Invalid selection", "The entered text did not match any of the valid targets");
        return;
    }

    QString targetIdentifier = m_ui->targetComboBox->currentData().toString();
    autoType()->performAutoTypeOnExternalPlugin(m_entry, m_pluginName, m_targetList.get(targetIdentifier));
}

AutoTypeTargetSelectDialog::~AutoTypeTargetSelectDialog() = default;
