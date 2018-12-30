/*
 *  Copyright (C) 2017 Toni Spets <toni.spets@iki.fi>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "AgentSettingsWidget.h"
#include "ui_AgentSettingsWidget.h"

#include "core/Config.h"

AgentSettingsWidget::AgentSettingsWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::AgentSettingsWidget())
{
    m_ui->setupUi(this);
#ifndef Q_OS_WIN
    m_ui->useOpenSSHCheckBox->setVisible(false);
#endif
}

AgentSettingsWidget::~AgentSettingsWidget()
{
}

void AgentSettingsWidget::loadSettings()
{
    m_ui->enableSSHAgentCheckBox->setChecked(config()->get("SSHAgent", false).toBool());
#ifdef Q_OS_WIN
    m_ui->useOpenSSHCheckBox->setChecked(config()->get("SSHAgentOpenSSH", false).toBool());
#endif
}

void AgentSettingsWidget::saveSettings()
{
    config()->set("SSHAgent", m_ui->enableSSHAgentCheckBox->isChecked());
#ifdef Q_OS_WIN
    config()->set("SSHAgentOpenSSH", m_ui->useOpenSSHCheckBox->isChecked());
#endif
}
