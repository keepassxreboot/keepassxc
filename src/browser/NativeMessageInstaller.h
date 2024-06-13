/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NATIVEMESSAGEINSTALLER_H
#define NATIVEMESSAGEINSTALLER_H

#include "BrowserShared.h"
#include <QtGlobal>

class QJsonObject;

class NativeMessageInstaller
{
public:
    NativeMessageInstaller() = default;

    void setBrowserEnabled(BrowserShared::SupportedBrowsers browser, bool enabled);
    bool isBrowserEnabled(BrowserShared::SupportedBrowsers browser);

    QString getProxyPath() const;
    QString getInstalledProxyPath() const;
    void updateBinaryPaths();

private:
    QString getTargetPath(BrowserShared::SupportedBrowsers browser) const;
    QString getBrowserName(BrowserShared::SupportedBrowsers browser) const;
    QString getNativeMessagePath(BrowserShared::SupportedBrowsers browser) const;
    QString getHostName(BrowserShared::SupportedBrowsers browser) const;
    QJsonObject constructFile(BrowserShared::SupportedBrowsers browser);
    bool createNativeMessageFile(BrowserShared::SupportedBrowsers browser);
    bool isFirefoxBrowser(BrowserShared::SupportedBrowsers browser) const;

    Q_DISABLE_COPY(NativeMessageInstaller);
};

#endif // NATIVEMESSAGEINSTALLER_H
