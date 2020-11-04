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

#include <QFile>

using FdoSecrets::Service;

// TODO: Only used for testing. Need to split service functions away from settings page.
QPointer<FdoSecretsPlugin> g_fdoSecretsPlugin;

FdoSecretsPlugin::FdoSecretsPlugin(DatabaseTabWidget* tabWidget)
    : m_dbTabs(tabWidget)
{
    g_fdoSecretsPlugin = this;
    FdoSecrets::registerDBusTypes();
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
            m_secretService = Service::Create(this, m_dbTabs);
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

QString FdoSecretsPlugin::reportExistingService() const
{
    auto pidStr = tr("Unknown", "Unknown PID");
    auto exeStr = tr("Unknown", "Unknown executable path");

    // try get pid
    auto pid = QDBusConnection::sessionBus().interface()->servicePid(DBUS_SERVICE_SECRET);
    if (pid.isValid()) {
        pidStr = QString::number(pid.value());

        // try get the first part of the cmdline, which usually is the executable name/path
        QFile proc(QStringLiteral("/proc/%1/cmdline").arg(pid.value()));
        if (proc.open(QFile::ReadOnly)) {
            auto parts = proc.readAll().split('\0');
            if (parts.length() >= 1) {
                exeStr = QString::fromLocal8Bit(parts[0]).trimmed();
            }
        }
    }
    auto otherService = tr("<i>PID: %1, Executable: %2</i>", "<i>PID: 1234, Executable: /path/to/exe</i>")
                            .arg(pidStr, exeStr.toHtmlEscaped());
    return tr("Another secret service is running (%1).<br/>"
              "Please stop/remove it before re-enabling the Secret Service Integration.")
        .arg(otherService);
}
