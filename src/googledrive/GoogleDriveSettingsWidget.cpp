/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "GoogleDriveSettingsWidget.h"
#include "GoogleDriveService.h"

#include "core/Config.h"
#include "config-keepassx.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

GoogleDriveSettingsWidget::GoogleDriveSettingsWidget(QWidget* parent)
    : QWidget(parent)
    , m_service(new GoogleDriveService(this))
{
    auto* mainLayout = new QVBoxLayout(this);

    // --- Account section ---
    auto* accountGroup = new QGroupBox(tr("Google Account"), this);
    auto* accountLayout = new QVBoxLayout(accountGroup);

    m_statusLabel = new QLabel(tr("Status: Not connected"), this);
    accountLayout->addWidget(m_statusLabel);

    auto* buttonLayout = new QHBoxLayout();
    m_connectButton = new QPushButton(tr("Connect to Google Drive"), this);
    m_disconnectButton = new QPushButton(tr("Disconnect"), this);
    m_disconnectButton->setEnabled(false);
    buttonLayout->addWidget(m_connectButton);
    buttonLayout->addWidget(m_disconnectButton);
    buttonLayout->addStretch();
    accountLayout->addLayout(buttonLayout);

    mainLayout->addWidget(accountGroup);

    // --- API Credentials section ---
    auto* credsGroup = new QGroupBox(tr("API Credentials"), this);
    auto* credsLayout = new QVBoxLayout(credsGroup);

    auto* infoLabel = new QLabel(
        tr("To connect, create a Google Cloud project and enable the Drive API:\n"
           "1. Go to https://console.developers.google.com/\n"
           "2. Create a project \u2192 Enable the Google Drive API\n"
           "3. Create OAuth 2.0 credentials (Desktop Application type)\n"
           "4. Add http://localhost:18080/ as an authorized redirect URI\n"
           "5. Copy the Client ID and Client Secret below"),
        this);
    infoLabel->setWordWrap(true);
    infoLabel->setOpenExternalLinks(true);
    credsLayout->addWidget(infoLabel);

    auto* clientIdLayout = new QHBoxLayout();
    clientIdLayout->addWidget(new QLabel(tr("Client ID:"), this));
    m_clientIdEdit = new QLineEdit(this);
    m_clientIdEdit->setPlaceholderText(tr("Paste your Google OAuth Client ID here"));
    clientIdLayout->addWidget(m_clientIdEdit, 1);
    credsLayout->addLayout(clientIdLayout);

    auto* clientSecretLayout = new QHBoxLayout();
    clientSecretLayout->addWidget(new QLabel(tr("Client Secret:"), this));
    m_clientSecretEdit = new QLineEdit(this);
    m_clientSecretEdit->setEchoMode(QLineEdit::Password);
    m_clientSecretEdit->setPlaceholderText(tr("Paste your Google OAuth Client Secret here"));
    clientSecretLayout->addWidget(m_clientSecretEdit, 1);
    credsLayout->addLayout(clientSecretLayout);

    mainLayout->addWidget(credsGroup);

    mainLayout->addStretch();

    connect(m_connectButton, &QPushButton::clicked, this, &GoogleDriveSettingsWidget::connectClicked);
    connect(m_disconnectButton, &QPushButton::clicked, this, &GoogleDriveSettingsWidget::disconnectClicked);
    connect(m_service, &GoogleDriveService::authStatusChanged, this, &GoogleDriveSettingsWidget::onAuthStatusChanged);
    connect(m_service, &GoogleDriveService::authFailed, this, &GoogleDriveSettingsWidget::onAuthFailed);
}

GoogleDriveSettingsWidget::~GoogleDriveSettingsWidget() = default;

void GoogleDriveSettingsWidget::loadSettings()
{
    QString storedId = config()->get(Config::GoogleDrive_ClientId).toString();
    QString storedSecret = config()->get(Config::GoogleDrive_ClientSecret).toString();

    m_clientIdEdit->setText(storedId);
    m_clientSecretEdit->setText(storedSecret);

    if (storedId.isEmpty() && storedSecret.isEmpty()) {
#ifdef GOOGLEDRIVE_CLIENT_ID
        m_clientIdEdit->setPlaceholderText(tr("Using built-in credentials (override if needed)"));
        m_clientSecretEdit->setPlaceholderText(tr("Built-in secret configured"));
#endif
    }

    bool authenticated = m_service->isAuthenticated();
    m_statusLabel->setText(authenticated ? tr("Status: Connected") : tr("Status: Not connected"));
    m_connectButton->setEnabled(!authenticated);
    m_disconnectButton->setEnabled(authenticated);
}

void GoogleDriveSettingsWidget::saveSettings()
{
    config()->set(Config::GoogleDrive_ClientId, m_clientIdEdit->text());
    config()->set(Config::GoogleDrive_ClientSecret, m_clientSecretEdit->text());
    config()->sync();
}

void GoogleDriveSettingsWidget::connectClicked()
{
    saveSettings();

    bool hasManualId = !m_clientIdEdit->text().isEmpty();
    bool hasBuiltIn = false;
#ifdef GOOGLEDRIVE_CLIENT_ID
    hasBuiltIn = true;
#endif

    if (!hasManualId && !hasBuiltIn) {
        m_statusLabel->setText(tr("Status: Enter your Client ID in the API Credentials section first"));
        return;
    }

    m_statusLabel->setText(tr("Status: Authenticating..."));
    m_connectButton->setEnabled(false);
    m_service->authenticate();
}

void GoogleDriveSettingsWidget::disconnectClicked()
{
    m_service->disconnect();
}

void GoogleDriveSettingsWidget::onAuthStatusChanged(bool authenticated)
{
    m_statusLabel->setText(authenticated ? tr("Status: Connected") : tr("Status: Not connected"));
    m_connectButton->setEnabled(!authenticated);
    m_disconnectButton->setEnabled(authenticated);
}

void GoogleDriveSettingsWidget::onAuthFailed(const QString& error)
{
    m_statusLabel->setText(error.isEmpty()
        ? tr("Status: Authentication failed")
        : tr("Status: %1").arg(error));
    m_connectButton->setEnabled(true);
    m_disconnectButton->setEnabled(false);
}
