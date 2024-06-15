/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2000-2008 Tom Sato <VEF00200@nifty.ne.jp>
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

#ifndef KEEPASSX_AUTOTYPEWAYLAND_H
#define KEEPASSX_AUTOTYPEWAYLAND_H

#include <QApplication>
#include <QSet>
#include <QWidget>
#include <QtPlugin>
#include <qdbusconnection.h>
#include <qdbusinterface.h>
#include <qdbusmessage.h>
#include <qnamespace.h>
#include <qpointer.h>
#include <qscopedpointer.h>
#include <qsocketnotifier.h>
#include <xkbcommon/xkbcommon.h>

#include "autotype/AutoTypePlatformPlugin.h"

class AutoTypePlatformWayland : public QObject, public AutoTypePlatformInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.keepassx.AutoTypePlatformWaylnd")
    Q_INTERFACES(AutoTypePlatformInterface)

public:
    AutoTypePlatformWayland();
    bool isAvailable() override;
    void unload() override;
    QStringList windowTitles() override;
    WId activeWindow() override;
    QString activeWindowTitle() override;
    bool raiseWindow(WId window) override;
    AutoTypeExecutor* createExecutor() override;

    AutoTypeAction::Result sendKey(xkb_keysym_t keysym, QVector<xkb_keysym_t> modifiers = {});
    void createSession();

private:
    bool m_loaded;
    QDBusConnection m_bus;
    QMap<QString, std::function<void(uint, QVariantMap)>> m_handlers;
    QDBusInterface m_remote_desktop;
    QDBusObjectPath m_session_handle;
    QString m_restore_token;
    bool m_session_started = false;

    void handleCreateSession(uint response, QVariantMap results);
    void handleSelectDevices(uint response, QVariantMap results);
    void handleStart(uint response, QVariantMap results);
private slots:
    void portalResponse(uint response, QVariantMap results, QDBusMessage message);
};

class AutoTypeExecutorWayland : public AutoTypeExecutor
{
public:
    explicit AutoTypeExecutorWayland(AutoTypePlatformWayland* platform);

    AutoTypeAction::Result execBegin(const AutoTypeBegin* action) override;
    AutoTypeAction::Result execType(const AutoTypeKey* action) override;
    AutoTypeAction::Result execClearField(const AutoTypeClearField* action) override;

private:
    AutoTypePlatformWayland* const m_platform;
};

#endif // KEEPASSX_AUTOTYPEWAYLAND_H
