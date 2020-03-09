/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_RESOURCES_H
#define KEEPASSX_RESOURCES_H

#include <QHash>
#include <QIcon>
#include <QString>

class Resources
{
public:
    QString dataPath(const QString& name);
    QString pluginPath(const QString& name);
    QString wordlistPath(const QString& name);
    QIcon applicationIcon();
    QIcon trayIcon();
    QIcon trayIconLocked();
    QIcon trayIconUnlocked();
    QIcon icon(const QString& name, bool recolor = true);
    QIcon onOffIcon(const QString& name, bool recolor = true);

    static Resources* instance();

private:
    Resources();
    bool testResourceDir(const QString& dir);
    bool useDarkIcon();

    static Resources* m_instance;

    QString m_dataPath;
    QHash<QString, QIcon> m_iconCache;

    Q_DISABLE_COPY(Resources)
};

inline Resources* resources()
{
    return Resources::instance();
}

#endif // KEEPASSX_RESOURCES_H
