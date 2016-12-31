#include "ScreenLockListenerDBus.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

ScreenLockListenerDBus::ScreenLockListenerDBus(QWidget *parent):
    ScreenLockListenerPrivate(parent)
{
    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    QDBusConnection systemBus = QDBusConnection::systemBus();
    sessionBus.connect(
                "org.gnome.SessionManager", // service
                "/org/gnome/SessionManager/Presence", // path
                "org.gnome.SessionManager.Presence", // interface
                "StatusChanged", // signal name
                this, //receiver
                SLOT(gnomeSessionStatusChanged(uint)));

    systemBus.connect(
                "org.freedesktop.login1", // service
                "/org/freedesktop/login1", // path
                "org.freedesktop.login1.Manager", // interface
                "PrepareForSleep", // signal name
                this, //receiver
                SLOT(logindPrepareForSleep(bool)));

    sessionBus.connect(
                "com.canonical.Unity", // service
                "/com/canonical/Unity/Session", // path
                "com.canonical.Unity.Session", // interface
                "Locked", // signal name
                this, //receiver
                SLOT(unityLocked()));

    /* Currently unable to get the current user session from login1.Manager
    QDBusInterface login1_manager_iface("org.freedesktop.login1", "/org/freedesktop/login1",
                              "org.freedesktop.login1.Manager", systemBus);
    if(login1_manager_iface.isValid()){
        qint64 my_pid = QCoreApplication::applicationPid();
        QDBusReply<QString> reply = login1_manager_iface.call("GetSessionByPID",static_cast<quint32>(my_pid));
        if (reply.isValid()){
            QString current_session = reply.value();
        }
    }
    */
}

void ScreenLockListenerDBus::gnomeSessionStatusChanged(uint status)
{
    if(status != 0){
        Q_EMIT screenLocked();
    }
}

void ScreenLockListenerDBus::logindPrepareForSleep(bool beforeSleep)
{
    if(beforeSleep){
        Q_EMIT screenLocked();
    }
}

void ScreenLockListenerDBus::unityLocked()
{
    Q_EMIT screenLocked();
}
