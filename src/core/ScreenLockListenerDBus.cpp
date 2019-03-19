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

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QProcessEnvironment>

ScreenLockListenerDBus::ScreenLockListenerDBus(QWidget* parent)
    : ScreenLockListenerPrivate(parent)
{
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

    systemBus.connect("org.freedesktop.login1", // service
                      "/org/freedesktop/login1", // path
                      "org.freedesktop.login1.Manager", // interface
                      "PrepareForSleep", // signal name
                      this, // receiver
                      SLOT(logindPrepareForSleep(bool)));

    QString sessionId = QProcessEnvironment::systemEnvironment().value("XDG_SESSION_ID");
    systemBus.connect("", // service
                      QString("/org/freedesktop/login1/session/") + sessionId, // path
                      "org.freedesktop.login1.Session", // interface
                      "Lock", // signal name
                      this, // receiver
                      SLOT(unityLocked()));

    sessionBus.connect("com.canonical.Unity", // service
                       "/com/canonical/Unity/Session", // path
                       "com.canonical.Unity.Session", // interface
                       "Locked", // signal name
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
