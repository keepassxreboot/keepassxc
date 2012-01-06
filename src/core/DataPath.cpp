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

#include "DataPath.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

#include "config-keepassx.h"

QString DataPath::path(const QString& name)
{
    return m_basePath + name;
}

QIcon DataPath::applicationIcon()
{
    QIcon icon = QIcon::fromTheme("keepassx");

#if defined(QT_DEBUG) || defined(Q_WS_MAC) || defined(Q_WS_WIN)
    if (icon.isNull()) {
        QStringList pngSizes;
        pngSizes << "16" << "24" << "32" << "48" << "64" << "128";
        Q_FOREACH (const QString& size, pngSizes) {
            icon.addFile(QString("%1/icons/application/%2x%2/apps/keepassx.png").arg(m_basePath, size));
        }
        icon.addFile(QString("%1/icons/application/scalable/apps/keepassx.svgz").arg(m_basePath));
    }
#endif

    return icon;
}

DataPath::DataPath()
{
    if (false) {
    }
#ifdef QT_DEBUG
    else if (testSetDir(QString(KEEPASSX_SOURCE_DIR) + "/share")) {
    }
#endif
#ifdef Q_WS_X11
    else if (testSetDir(QCoreApplication::applicationDirPath() + "/../share/keepassx")) {
    }
#endif
#ifdef Q_WS_MAC
    else if (testSetDir(QCoreApplication::applicationDirPath() + "/../Resources/keepassx")) {
    }
#endif
#ifdef Q_WS_WIN
    else if (testSetDir(QCoreApplication::applicationDirPath() + "/share")) {
    }
#endif

    if (m_basePath.isEmpty()) {
        qWarning("DataPath::DataPath: can't find data dir");
    }
    else {
        m_basePath = QDir::cleanPath(m_basePath) + "/";
    }
}

bool DataPath::testSetDir(const QString& dir)
{
    if (QFile::exists(dir + "/icons/database/C00_Password.png")) {
        m_basePath = dir;
        return true;
    }
    else {
        return false;
    }
}

DataPath* dataPath()
{
    static DataPath* instance = 0;

    if (!instance) {
        instance = new DataPath();
    }

    return instance;
}
