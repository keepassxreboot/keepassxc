/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#include "DatabaseSettingsWidgetRemote.h"
#include "ui_DatabaseSettingsWidgetRemote.h"

#include "core/Global.h"
#include "core/Metadata.h"

#include "RemoteSettings.h"

DatabaseSettingsWidgetRemote::DatabaseSettingsWidgetRemote(QWidget* parent)
    : DatabaseSettingsWidget(parent)
    , m_customDataHandler(nullptr)
    , m_ui(new Ui::DatabaseSettingsWidgetRemote())
{
    m_ui->setupUi(this);
    m_ui->messageWidget->setHidden(true);

    connect(m_ui->saveSettingsButton, &QPushButton::clicked, this, &DatabaseSettingsWidgetRemote::saveCurrentSettings);
    connect(
        m_ui->removeSettingsButton, &QPushButton::clicked, this, &DatabaseSettingsWidgetRemote::removeCurrentSettings);
    connect(m_ui->settingsListWidget,
            &QListWidget::itemSelectionChanged,
            this,
            &DatabaseSettingsWidgetRemote::editCurrentSettings);
}

DatabaseSettingsWidgetRemote::~DatabaseSettingsWidgetRemote() = default;

void DatabaseSettingsWidgetRemote::initialize()
{
    clearFields();
    m_customDataHandler = new RemoteSettingsCustomDataHandler(this, m_db);
    updateSettingsList();
    if (!m_customDataHandler->getRemoteProgramEntries().empty()) {
        m_ui->settingsListWidget->setCurrentRow(0);
        m_ui->removeSettingsButton->setEnabled(true);
    } else {
        m_ui->removeSettingsButton->setDisabled(true);
    }
}

void DatabaseSettingsWidgetRemote::uninitialize()
{
}

bool DatabaseSettingsWidgetRemote::save()
{
    m_customDataHandler->syncConfig();
    return true;
}

void DatabaseSettingsWidgetRemote::saveCurrentSettings()
{
    QString name = m_ui->nameLineEdit->text();
    if (name.isEmpty()) {
        m_ui->messageWidget->showMessage(tr("Name cannot be empty."), MessageWidget::Warning);
        return;
    }

    auto* remoteSettings = new RemoteSettings();
    remoteSettings->setName(m_ui->nameLineEdit->text());
    remoteSettings->setDownloadCommand(m_ui->downloadCommand->text());
    remoteSettings->setDownloadCommandInput(m_ui->inputForDownload->toPlainText());
    remoteSettings->setUploadCommand(m_ui->uploadCommand->text());
    remoteSettings->setUploadCommandInput(m_ui->inputForUpload->toPlainText());

    m_db->publicCustomData();

    m_customDataHandler->addRemoteSettingsEntry(remoteSettings);
    updateSettingsList();

    auto item = findItemByName(name);
    m_ui->settingsListWidget->setCurrentItem(item);
    m_ui->removeSettingsButton->setEnabled(true);
}

QListWidgetItem* DatabaseSettingsWidgetRemote::findItemByName(const QString& name)
{
    return m_ui->settingsListWidget->findItems(name, Qt::MatchExactly).first();
}

void DatabaseSettingsWidgetRemote::removeCurrentSettings()
{
    m_customDataHandler->removeRemoteSettingsEntry(m_ui->nameLineEdit->text());
    updateSettingsList();
    if (!m_customDataHandler->getRemoteProgramEntries().empty()) {
        m_ui->settingsListWidget->setCurrentRow(0);
        m_ui->removeSettingsButton->setEnabled(true);
    } else {
        clearFields();
        m_ui->removeSettingsButton->setDisabled(true);
    }
}

void DatabaseSettingsWidgetRemote::editCurrentSettings()
{
    if (m_ui->settingsListWidget->currentItem() == nullptr) {
        return;
    }

    QString itemName = m_ui->settingsListWidget->currentItem()->text();
    auto* remoteSettings = m_customDataHandler->getRemoteSettingsEntry(itemName);
    if (remoteSettings == nullptr) {
        // this should never happen
        return;
    }

    m_ui->nameLineEdit->setText(remoteSettings->getName());
    m_ui->downloadCommand->setText(remoteSettings->getDownloadCommand());
    m_ui->inputForDownload->setPlainText(remoteSettings->getDownloadCommandInput());
    m_ui->uploadCommand->setText(remoteSettings->getUploadCommand());
    m_ui->inputForUpload->setPlainText(remoteSettings->getUploadCommandInput());
}

void DatabaseSettingsWidgetRemote::updateSettingsList()
{
    m_ui->settingsListWidget->clear();
    foreach (auto remoteSetting, m_customDataHandler->getRemoteProgramEntries()) {
        auto* item = new QListWidgetItem(m_ui->settingsListWidget);
        item->setText(remoteSetting->getName());
        m_ui->settingsListWidget->addItem(item);
    }
}

void DatabaseSettingsWidgetRemote::clearFields()
{
    m_ui->nameLineEdit->setText("");
    m_ui->downloadCommand->setText("");
    m_ui->inputForDownload->setPlainText("");
    m_ui->uploadCommand->setText("");
    m_ui->inputForUpload->setPlainText("");
}
