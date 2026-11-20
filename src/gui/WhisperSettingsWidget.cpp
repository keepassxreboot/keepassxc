/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "WhisperSettingsWidget.h"
#include "ui_WhisperSettingsWidget.h"

#include "core/Config.h"

WhisperSettingsWidget::WhisperSettingsWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::WhisperSettingsWidget())
{
    m_ui->setupUi(this);
    m_ui->serverDescriptionLabel->setForegroundRole(QPalette::PlaceholderText);
    connect(m_ui->resetUrlButton, &QPushButton::clicked, this, [this] {
        m_ui->serverUrlLineEdit->setText(config()->getDefault(Config::Whisper_ServerUrl).toString());
    });
}

WhisperSettingsWidget::~WhisperSettingsWidget() = default;

void WhisperSettingsWidget::loadSettings()
{
    m_ui->serverUrlLineEdit->setText(config()->get(Config::Whisper_ServerUrl).toString());
    m_ui->confirmShareCheckBox->setChecked(!config()->get(Config::Whisper_ConsentGiven).toBool());
}

void WhisperSettingsWidget::saveSettings()
{
    config()->set(Config::Whisper_ServerUrl, m_ui->serverUrlLineEdit->text().trimmed());
    config()->set(Config::Whisper_ConsentGiven, !m_ui->confirmShareCheckBox->isChecked());
}
