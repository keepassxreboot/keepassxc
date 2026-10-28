/*
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

#include "ScreenLockListenerDBus.h"

#include "core/Config.h"
#include "gui/DatabaseWidget.h"
#include "gui/MainWindow.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDebug>
#include <QProcessEnvironment>

ScreenLockListenerDBus::ScreenLockListenerDBus(QWidget* parent)
    : ScreenLockListenerPrivate(parent)
{
    m_mainWindow = qobject_cast<MainWindow*>(parent);

    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    QDBusConnection systemBus = QDBusConnection::systemBus();

    sessionBus.connect("org.freedesktop.ScreenSaver", // service
                       "/org/freedesktop/ScreenSaver", // path
                       "org.freedesktop.ScreenSaver", // interface
                       "ActiveChanged", // signal name
                       this, // receiver
                       SLOT(freedesktopScreenSaver(bool)));

    sessionBus.connect("org.gnome.ScreenSaver", // service
                       "/org/gnome/ScreenSaver", // path
                       "org.gnome.ScreenSaver", // interface
                       "ActiveChanged", // signal name
                       this, // receiver
                       SLOT(freedesktopScreenSaver(bool)));

    sessionBus.connect("org.gnome.SessionManager", // service
                       "/org/gnome/SessionManager/Presence", // path
                       "org.gnome.SessionManager.Presence", // interface
                       "StatusChanged", // signal name
                       this, // receiver
                       SLOT(gnomeSessionStatusChanged(uint)));

    sessionBus.connect("org.xfce.ScreenSaver", // service
                       "/org/xfce/ScreenSaver", // path
                       "org.xfce.ScreenSaver", // interface
                       "ActiveChanged", // signal name
                       this, // receiver
                       SLOT(freedesktopScreenSaver(bool)));

    systemBus.connect("org.freedesktop.login1", // service
                      "/org/freedesktop/login1", // path
                      "org.freedesktop.login1.Manager", // interface
                      "PrepareForSleep", // signal name
                      this, // receiver
                      SLOT(logindPrepareForSleep(bool)));

    QString sessionId = QProcessEnvironment::systemEnvironment().value("XDG_SESSION_ID");
    QDBusInterface loginManager("org.freedesktop.login1", // service
                                "/org/freedesktop/login1", // path
                                "org.freedesktop.login1.Manager", // interface
                                systemBus);
    if (loginManager.isValid()) {
        QList<QVariant> args = {sessionId};
        loginManager.callWithCallback("GetSession", args, this, SLOT(login1SessionObjectReceived(QDBusMessage)));
    }

    sessionBus.connect("com.canonical.Unity", // service
                       "/com/canonical/Unity/Session", // path
                       "com.canonical.Unity.Session", // interface
                       "Locked", // signal name
                       this, // receiver
                       SLOT(unityLocked()));

    connect(m_mainWindow, &MainWindow::databaseUnlocked, this, &ScreenLockListenerDBus::refreshInhibitor);
    connect(m_mainWindow, &MainWindow::databaseLocked, this, &ScreenLockListenerDBus::refreshInhibitor);
    connect(config(), &Config::changed, this, &ScreenLockListenerDBus::refreshInhibitor);

    refreshInhibitor();
}

void ScreenLockListenerDBus::login1SessionObjectReceived(QDBusMessage response)
{
    if (response.arguments().isEmpty()) {
        qDebug() << "org.freedesktop.login1.Manager.GetSession did not return results";
        return;
    }
    QVariant arg0 = response.arguments().at(0);
    if (!arg0.canConvert<QDBusObjectPath>()) {
        qDebug() << "org.freedesktop.login1.Manager.GetSession did not return a QDBusObjectPath";
        return;
    }
    auto path = arg0.value<QDBusObjectPath>();
    QDBusConnection systemBus = QDBusConnection::systemBus();

    systemBus.connect("", // service
                      path.path(), // path
                      "org.freedesktop.login1.Session", // interface
                      "Lock", // signal name
                      this, // receiver
                      SLOT(unityLocked()));
}

void ScreenLockListenerDBus::gnomeSessionStatusChanged(uint status)
{
    if (status != 0) {
        emit screenLocked();
    }
}

void ScreenLockListenerDBus::logindPrepareForSleep(bool beforeSleep)
{
    if (beforeSleep) {
        emit screenLocked();
    }
}

void ScreenLockListenerDBus::unityLocked()
{
    emit screenLocked();
}

void ScreenLockListenerDBus::freedesktopScreenSaver(bool status)
{
    if (status) {
        emit screenLocked();
    }
}

void ScreenLockListenerDBus::refreshInhibitor()
{
    const bool hold = config()->get(Config::Security_LockDatabaseScreenLock).toBool() && anyDatabaseUnlocked();

    if (hold && !m_pipeFd.isValid()) {
        acquireInhibitor();
    } else if (!hold && m_pipeFd.isValid()) {
        releaseInhibitor();
    }
}

bool ScreenLockListenerDBus::anyDatabaseUnlocked() const
{
    if (!m_mainWindow) {
        return false;
    }
    for (const auto* dbWidget : m_mainWindow->getOpenDatabases()) {
        if (dbWidget && !dbWidget->isLocked()) {
            return true;
        }
    }
    return false;
}

void ScreenLockListenerDBus::acquireInhibitor()
{
    QDBusInterface loginManager("org.freedesktop.login1", // service
                                "/org/freedesktop/login1", // path
                                "org.freedesktop.login1.Manager", // interface
                                QDBusConnection::systemBus());
    if (!loginManager.isValid()) {
        qWarning() << "org.freedesktop.login1.Manager is not available, sleep inhibitor not acquired";
        return;
    }

    const QDBusMessage reply =
        loginManager.call("Inhibit",
                          QLatin1String("sleep"), // what
                          QLatin1String("KeePassXC"), // who
                          QLatin1String("Waiting for open databases to lock before sleep"), // why
                          QLatin1String("delay")); // mode
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "Failed to acquire sleep inhibitor:" << reply.errorMessage();
        return;
    }

    m_pipeFd = reply.arguments().at(0).value<QDBusUnixFileDescriptor>();
}

void ScreenLockListenerDBus::releaseInhibitor()
{
    // Closing the pipe fd releases the inhibitor in logind
    m_pipeFd = QDBusUnixFileDescriptor();
}
