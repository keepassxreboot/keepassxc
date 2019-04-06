/*
 *  Copyright (C) 2017 Sami Vänttinen <sami.vanttinen@protonmail.com>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef HOSTINSTALLER_H
#define HOSTINSTALLER_H

#include <QJsonObject>
#include <QObject>
#include <QSettings>

class HostInstaller : public QObject
{
    Q_OBJECT

public:
    enum SupportedBrowsers : int
    {
        CHROME = 0,
        CHROMIUM = 1,
        FIREFOX = 2,
        VIVALDI = 3,
        TOR_BROWSER = 4,
        BRAVE = 5
    };

public:
    HostInstaller();
    bool checkIfInstalled(SupportedBrowsers browser);
    bool checkIfProxyExists(const bool& proxy, const QString& location, QString& path) const;
    void installBrowser(SupportedBrowsers browser,
                        const bool& enabled,
                        const bool& proxy = false,
                        const QString& location = "");
    void updateBinaryPaths(const bool& proxy, const QString& location = "");

private:
    QString getTargetPath(SupportedBrowsers browser) const;
    QString getBrowserName(SupportedBrowsers browser) const;
    QString getPath(SupportedBrowsers browser) const;
    QString getInstallDir(SupportedBrowsers browser) const;
    QString getProxyPath(const bool& proxy, const QString& location) const;
    QJsonObject constructFile(SupportedBrowsers browser, const bool& proxy, const QString& location);
    bool registryEntryFound(const QSettings& settings);
    bool saveFile(SupportedBrowsers browser, const QJsonObject& script);

private:
    const QString HOST_NAME;
    const QStringList ALLOWED_EXTENSIONS;
    const QStringList ALLOWED_ORIGINS;
    const QString TARGET_DIR_CHROME;
    const QString TARGET_DIR_CHROMIUM;
    const QString TARGET_DIR_FIREFOX;
    const QString TARGET_DIR_VIVALDI;
    const QString TARGET_DIR_TOR_BROWSER;
    const QString TARGET_DIR_BRAVE;
};

#endif // HOSTINSTALLER_H
