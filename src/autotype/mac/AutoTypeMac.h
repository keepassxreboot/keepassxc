/*
 *  Copyright (C) 2016 Lennart Glauer <mail@lennart-glauer.de>
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

#ifndef KEEPASSX_AUTOTYPEMAC_H
#define KEEPASSX_AUTOTYPEMAC_H

#include <Carbon/Carbon.h>
#include <QtPlugin>
#include <memory>

#include "autotype/AutoTypePlatformPlugin.h"
#include "autotype/AutoTypeAction.h"

class AutoTypePlatformMac : public QObject, public AutoTypePlatformInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.keepassx.AutoTypePlatformMac")
    Q_INTERFACES(AutoTypePlatformInterface)

public:
    AutoTypePlatformMac();
    bool isAvailable() override;
    QStringList windowTitles() override;
    WId activeWindow() override;
    QString activeWindowTitle() override;
    bool raiseWindow(WId pid) override;
    AutoTypeExecutor* createExecutor() override;

    bool hideOwnWindow() override;
    bool raiseOwnWindow() override;

    void sendChar(const QChar& ch, bool isKeyDown);
    void sendKey(Qt::Key key, bool isKeyDown, Qt::KeyboardModifiers modifiers);

private:
    static int windowLayer(CFDictionaryRef window);
    static QString windowTitle(CFDictionaryRef window);
};

class AutoTypeExecutorMac : public AutoTypeExecutor
{
public:
    explicit AutoTypeExecutorMac(AutoTypePlatformMac* platform);

    void execChar(AutoTypeChar* action) override;
    void execKey(AutoTypeKey* action) override;
    void execClearField(AutoTypeClearField* action) override;

private:
    AutoTypePlatformMac* const m_platform;
};

#endif  // KEEPASSX_AUTOTYPEMAC_H
