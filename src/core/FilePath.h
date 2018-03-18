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

#ifndef KEEPASSX_FILEPATH_H
#define KEEPASSX_FILEPATH_H

#include <QHash>
#include <QIcon>
#include <QString>

class FilePath
{
public:
    QString dataPath(const QString& name);
    QString pluginPath(const QString& name);
    QString wordlistPath(const QString& name);
    QIcon applicationIcon();
    QIcon trayIcon();
    QIcon trayIconLocked();
    QIcon trayIconUnlocked();
    QIcon icon(const QString& category, const QString& name, bool fromTheme = true);
    QIcon onOffIcon(const QString& category, const QString& name);

    static FilePath* instance();

private:
    FilePath();
    bool testSetDir(const QString& dir);
    bool useDarkIcon();

    static FilePath* m_instance;

    QString m_dataPath;
    QHash<QString, QIcon> m_iconCache;

    Q_DISABLE_COPY(FilePath)
};

inline FilePath* filePath() {
    return FilePath::instance();
}

#endif // KEEPASSX_FILEPATH_H
