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
#include "fdosecrets/objects/Service.h"
#include "fdosecrets/widgets/SettingsWidgetFdoSecrets.h"

#include "gui/DatabaseTabWidget.h"

using FdoSecrets::DBusMgr;
using FdoSecrets::Service;

// TODO: Only used for testing. Need to split service functions away from settings page.
QPointer<FdoSecretsPlugin> g_fdoSecretsPlugin;

FdoSecretsPlugin::FdoSecretsPlugin(DatabaseTabWidget* tabWidget)
    : m_dbTabs(tabWidget)
    , m_dbus(new DBusMgr())
{
    registerDBusTypes(m_dbus);
    m_dbus->populateMethodCache();

    connect(m_dbus.data(), &DBusMgr::error, this, &FdoSecretsPlugin::emitError);
    g_fdoSecretsPlugin = this;
}

FdoSecretsPlugin* FdoSecretsPlugin::getPlugin()
{
    return g_fdoSecretsPlugin;
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
            m_secretService = Service::Create(this, m_dbTabs, m_dbus);
            if (!m_secretService) {
                FdoSecrets::settings()->setEnabled(false);
                return;
            }
            emit secretServiceStarted();
        }
    } else {
        if (m_secretService) {
            m_secretService.reset();
            emit secretServiceStopped();
        }
    }
}

Service* FdoSecretsPlugin::serviceInstance() const
{
    return m_secretService.data();
}

DatabaseTabWidget* FdoSecretsPlugin::dbTabs() const
{
    return m_dbTabs;
}

const QSharedPointer<FdoSecrets::DBusMgr>& FdoSecretsPlugin::dbus() const
{
    return m_dbus;
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

void FdoSecretsPlugin::emitError(const QString& msg)
{
    emit error(tr("<b>Fdo Secret Service:</b> %1").arg(msg));
    qDebug() << msg;
}
