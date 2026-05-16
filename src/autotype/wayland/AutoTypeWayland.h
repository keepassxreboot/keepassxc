/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_AUTOTYPEWAYLAND_H
#define KEEPASSXC_AUTOTYPEWAYLAND_H

#include <QtPlugin>

#include "autotype/AutoTypePlatformPlugin.h"

class AutoTypeExecutorWayland;
class NixUtils;

class AutoTypePlatformWayland : public QObject, public AutoTypePlatformInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.keepassxc.AutoTypePlatformWayland")
    Q_INTERFACES(AutoTypePlatformInterface)

public:
    AutoTypePlatformWayland() = default;
    void setOSUtils(OSUtilsBase* osUtils) override;
    bool isAvailable() override;
    QStringList windowTitles() override;
    WId activeWindow() override;
    QString activeWindowTitle() override;
    bool raiseWindow(WId window) override;
    bool hasWindowAccess() override
    {
        return false;
    };
    void unload() override;
    void prepareAutoType() override;
    void finishAutoType() override;

    AutoTypeExecutor* createExecutor() override;
    AutoTypeAction::Result sendKey(const AutoTypeKey*);

private:
    friend class AutoTypeExecutorWayland;

    NixUtils* m_nixUtils = nullptr;
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

#endif // KEEPASSXC_AUTOTYPEWAYLAND_H
