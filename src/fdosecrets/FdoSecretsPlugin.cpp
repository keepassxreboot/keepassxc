/*
 *  Copyright (C) 2018 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include "FdoSecretsPlugin.h"

#include "fdosecrets/FdoSecretsSettings.h"
#include "fdosecrets/objects/DBusTypes.h"
#include "fdosecrets/objects/Service.h"
#include "fdosecrets/widgets/SettingsWidgetFdoSecrets.h"

#include "gui/DatabaseTabWidget.h"

#include <utility>

using FdoSecrets::Service;

FdoSecretsPlugin::FdoSecretsPlugin(DatabaseTabWidget* tabWidget)
    : m_dbTabs(tabWidget)
{
    FdoSecrets::registerDBusTypes();
}

QWidget* FdoSecretsPlugin::createWidget()
{
    return new SettingsWidgetFdoSecrets(this);
}

void FdoSecretsPlugin::loadSettings(QWidget* widget)
{
    qobject_cast<SettingsWidgetFdoSecrets*>(widget)->loadSettings();
}

void FdoSecretsPlugin::saveSettings(QWidget* widget)
{
    qobject_cast<SettingsWidgetFdoSecrets*>(widget)->saveSettings();
    updateServiceState();
}

void FdoSecretsPlugin::updateServiceState()
{
    if (FdoSecrets::settings()->isEnabled()) {
        if (!m_secretService && m_dbTabs) {
            m_secretService.reset(new Service(this, m_dbTabs));
            connect(m_secretService.data(), &Service::error, this, [this](const QString& msg) {
                emit error(tr("Fdo Secret Service: %1").arg(msg));
            });
            if (!m_secretService->initialize()) {
                m_secretService.reset();
            }
        }
    } else {
        if (m_secretService) {
            m_secretService.reset();
        }
    }
}

Service* FdoSecretsPlugin::serviceInstance() const
{
    return m_secretService.data();
}

void FdoSecretsPlugin::emitRequestSwitchToDatabases()
{
    emit requestSwitchToDatabases();
}

void FdoSecretsPlugin::emitRequestShowNotification(const QString& msg, const QString& title)
{
    if (!FdoSecrets::settings()->showNotification()) {
        return;
    }
    emit requestShowNotification(msg, title, 10000);
}
