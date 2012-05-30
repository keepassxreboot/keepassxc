/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#include "Config.h"

#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtGui/QDesktopServices>

QVariant Config::get(const QString& key)
{
    return m_settings->value(key, m_defaults.value(key));
}

QVariant Config::get(const QString& key, const QVariant& defaultValue)
{
    return m_settings->value(key, defaultValue);
}

void Config::set(const QString& key, const QVariant& value)
{
    m_settings->setValue(key, value);
}

Config::Config()
{
    QString userPath;
    QString homePath = QDir::homePath();

#if defined(Q_WS_X11)
    // we can't use QDesktopServices on X11 as it uses XDG_DATA_HOME instead of XDG_CONFIG_HOME
    QByteArray env = qgetenv("XDG_CONFIG_HOME");
    if (env.isEmpty()) {
        userPath = homePath;
        userPath += "/.config";
    }
    else if (env[0] == '/') {
        userPath = QFile::decodeName(env);
    }
    else {
        userPath = homePath;
        userPath += '/';
        userPath += QFile::decodeName(env);
    }

    userPath += "/keepassx/";
#else
    userPath = QDir::fromNativeSeparators(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
    userPath += "/keepassx/";
#endif

    userPath += "keepassx2.ini";

    m_settings.reset(new QSettings(userPath, QSettings::IniFormat));

    m_defaults.insert("RememberLastDatabases", true);
    m_defaults.insert("ModifiedOnExpandedStateChanges", true);
    m_defaults.insert("security/clearclipboard", true);
    m_defaults.insert("security/clearclipboardtimeout", 10);
}

Config* config()
{
    static Config* instance(0);

    if (!instance) {
        instance = new Config();
    }

    return instance;
}
