/*
 *  Copyright (C) 2022 KeePassXC Team <team@keepassxc.org>
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

#include "FdoSecretsSettingsPage.h"

#include "FdoSecretsPlugin.h"
#include "fdosecrets/widgets/SettingsWidgetFdoSecrets.h"

#include "gui/DatabaseTabWidget.h"
#include "gui/DatabaseWidget.h"

FdoSecretsSettingsPage::FdoSecretsSettingsPage(FdoSecretsPlugin* plugin, DatabaseTabWidget* dbTabs)
    : QObject{plugin}
    , m_plugin{plugin}
    , m_dbTabs{dbTabs}
{
}

QWidget* FdoSecretsSettingsPage::createWidget()
{
    return new SettingsWidgetFdoSecrets(m_plugin, this);
}

void FdoSecretsSettingsPage::loadSettings(QWidget* widget)
{
    qobject_cast<SettingsWidgetFdoSecrets*>(widget)->loadSettings();
}

void FdoSecretsSettingsPage::saveSettings(QWidget* widget)
{
    qobject_cast<SettingsWidgetFdoSecrets*>(widget)->saveSettings();
}

void FdoSecretsSettingsPage::unlockDatabaseInDialog(DatabaseWidget* dbWidget)
{
    Q_ASSERT(dbWidget);
    // return immediately if the db is already unlocked
    if (dbWidget && !dbWidget->isLocked()) {
        return;
    }

    // actually show the dialog
    m_dbTabs->unlockDatabaseInDialog(dbWidget, DatabaseOpenDialog::Intent::None);
}

void FdoSecretsSettingsPage::switchToDatabaseSettings(DatabaseWidget* dbWidget)
{
    if (dbWidget->isLocked()) {
        return;
    }
    // switch selected to current
    m_dbTabs->setCurrentWidget(dbWidget);
    m_dbTabs->showDatabaseSettings();

    // open settings (switch from app settings to m_dbTabs)
    emit requestSwitchToDatabases();
}
