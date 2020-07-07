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

#include <QColor>
#include <QHash>
#include <QIcon>
#include <QString>

class Resources
{
public:
    QString dataPath(const QString& name) const;
    QString pluginPath(const QString& name) const;
    QString wordlistPath(const QString& name) const;
    QIcon applicationIcon();
    QIcon trayIcon();
    QIcon trayIconLocked();
    QIcon trayIconUnlocked();
    QString trayIconAppearance() const;
    QIcon icon(const QString& name, bool recolor = true, const QColor& overrideColor = QColor::Invalid);
    QIcon onOffIcon(const QString& name, bool recolor = true);

    static Resources* instance();

private:
    Resources();
    bool trySetResourceDir(const QString& path);

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
