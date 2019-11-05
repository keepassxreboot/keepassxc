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
#include "SSHAgent.h"
#include "ui_AgentSettingsWidget.h"

#include "core/Config.h"
#include <QProcessEnvironment>

AgentSettingsWidget::AgentSettingsWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::AgentSettingsWidget())
{
    m_ui->setupUi(this);
#ifndef Q_OS_WIN
    m_ui->useOpenSSHCheckBox->setVisible(false);
#else
    m_ui->sshAuthSockWidget->setVisible(false);
#endif
    auto sshAgentEnabled = config()->get("SSHAgent", false).toBool();
    m_ui->sshAuthSockMessageWidget->setVisible(sshAgentEnabled);
    m_ui->sshAuthSockMessageWidget->setCloseButtonVisible(false);
    m_ui->sshAuthSockMessageWidget->setAutoHideTimeout(-1);
}

AgentSettingsWidget::~AgentSettingsWidget()
{
}

void AgentSettingsWidget::loadSettings()
{
    auto sshAgentEnabled = config()->get("SSHAgent", false).toBool();
    m_ui->enableSSHAgentCheckBox->setChecked(sshAgentEnabled);
#ifdef Q_OS_WIN
    m_ui->useOpenSSHCheckBox->setChecked(config()->get("SSHAgentOpenSSH", false).toBool());
#else
    auto sshAuthSock = QProcessEnvironment::systemEnvironment().value("SSH_AUTH_SOCK");
    auto sshAuthSockOverride = config()->get("SSHAuthSockOverride", "").toString();
    m_ui->sshAuthSockLabel->setText(sshAuthSock.isEmpty() ? tr("(empty)") : sshAuthSock);
    m_ui->sshAuthSockOverrideEdit->setText(sshAuthSockOverride);
#endif

    if (sshAgentEnabled) {
        m_ui->sshAuthSockMessageWidget->setVisible(true);

#ifndef Q_OS_WIN
        if (sshAuthSock.isEmpty() && sshAuthSockOverride.isEmpty()) {
            m_ui->sshAuthSockMessageWidget->showMessage(
                tr("No SSH Agent socket available. Either make sure SSH_AUTH_SOCK environment variable exists or set "
                   "an override."),
                MessageWidget::Warning);
            return;
        }
#endif
        if (SSHAgent::instance()->testConnection()) {
            m_ui->sshAuthSockMessageWidget->showMessage(tr("SSH Agent connection is working!"),
                                                        MessageWidget::Positive);
        } else {
            m_ui->sshAuthSockMessageWidget->showMessage(SSHAgent::instance()->errorString(), MessageWidget::Error);
        }
    }
}

void AgentSettingsWidget::saveSettings()
{
    config()->set("SSHAgent", m_ui->enableSSHAgentCheckBox->isChecked());
    config()->set("SSHAuthSockOverride", m_ui->sshAuthSockOverrideEdit->text());
#ifdef Q_OS_WIN
    config()->set("SSHAgentOpenSSH", m_ui->useOpenSSHCheckBox->isChecked());
#endif
}
