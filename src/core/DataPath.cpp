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

DataPath* DataPath::m_instance(Q_NULLPTR);

QString DataPath::path(const QString& name)
{
    return m_basePath + name;
}

QIcon DataPath::applicationIcon()
{
    return icon("apps", "keepassx");
}

QIcon DataPath::icon(const QString& category, const QString& name, bool fromTheme)
{
    QIcon icon;

    if (fromTheme) {
        icon = QIcon::fromTheme(name);
    }

    if (icon.isNull()) {
        QStringList pngSizes;
        pngSizes << "16" << "22" << "24" << "32" << "48" << "64" << "128";
        QString filename;
        Q_FOREACH (const QString& size, pngSizes) {
            filename = QString("%1/icons/application/%2x%2/%3/%4.png").arg(m_basePath, size, category, name);
            if (QFile::exists(filename)) {
                icon.addFile(filename);
            }
        }
        filename = QString("%1/icons/application/scalable/%3/%4.svgz").arg(m_basePath, category, name);
        if (QFile::exists(filename)) {
            icon.addFile(filename);
        }
    }

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
    else if (testSetDir(QCoreApplication::applicationDirPath() + "/../Resources")) {
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

DataPath* DataPath::instance()
{
    if (!m_instance) {
        m_instance = new DataPath();
    }

    return m_instance;
}
