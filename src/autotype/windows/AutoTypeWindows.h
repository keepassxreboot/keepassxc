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

#ifndef KEEPASSX_AUTOTYPEWINDOWS_H
#define KEEPASSX_AUTOTYPEWINDOWS_H

#include <QtPlugin>
#include <windows.h>

#include "autotype/AutoTypeAction.h"
#include "autotype/AutoTypePlatformPlugin.h"

class AutoTypePlatformWin : public QObject, public AutoTypePlatformInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.keepassx.AutoTypePlatformWindows")
    Q_INTERFACES(AutoTypePlatformInterface)

public:
    bool isAvailable() override;
    QStringList windowTitles() override;
    WId activeWindow() override;
    QString activeWindowTitle() override;
    bool raiseWindow(WId window) override;
    AutoTypeExecutor* createExecutor() override;

    void sendChar(const QChar& ch, bool isKeyDown);
    void sendKey(Qt::Key key, bool isKeyDown);

private:
    static bool isExtendedKey(DWORD nativeKeyCode);
    static bool isAltTabWindow(HWND hwnd);
    static BOOL CALLBACK windowTitleEnumProc(_In_ HWND hwnd, _In_ LPARAM lParam);
    static QString windowTitle(HWND hwnd);
};

class AutoTypeExecutorWin : public AutoTypeExecutor
{
public:
    explicit AutoTypeExecutorWin(AutoTypePlatformWin* platform);

    void execChar(AutoTypeChar* action) override;
    void execKey(AutoTypeKey* action) override;
    void execClearField(AutoTypeClearField* action) override;

private:
    AutoTypePlatformWin* const m_platform;
};

#endif // KEEPASSX_AUTOTYPEWINDOWS_H
