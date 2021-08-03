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

#include "Resources.h"

#include <QCoreApplication>
#include <QDir>
#include <QLibrary>

#include "config-keepassx.h"
#include "core/Config.h"
#include "core/Global.h"

Resources* Resources::m_instance(nullptr);

QString Resources::dataPath(const QString& name) const
{
    if (name.isEmpty() || name.startsWith('/')) {
        return m_dataPath + name;
    }
    return m_dataPath + "/" + name;
}

QString Resources::pluginPath(const QString& name) const
{
    QStringList pluginPaths;

    QDir buildDir(QCoreApplication::applicationDirPath() + "/autotype");
    const QStringList buildDirEntryList = buildDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& dir : buildDirEntryList) {
        pluginPaths << QCoreApplication::applicationDirPath() + "/autotype/" + dir;
    }

    // for TestAutoType
    pluginPaths << QCoreApplication::applicationDirPath() + "/../src/autotype/test";

#if defined(Q_OS_MACOS) && defined(WITH_APP_BUNDLE)
    pluginPaths << QCoreApplication::applicationDirPath() + "/../PlugIns";
#endif

    pluginPaths << QCoreApplication::applicationDirPath();

    QString configuredPluginDir = KEEPASSX_PLUGIN_DIR;
    if (configuredPluginDir != ".") {
        if (QDir(configuredPluginDir).isAbsolute()) {
            pluginPaths << configuredPluginDir;
        } else {
            QString relativePluginDir =
                QStringLiteral("%1/../%2").arg(QCoreApplication::applicationDirPath(), configuredPluginDir);
            pluginPaths << QDir(relativePluginDir).canonicalPath();

            QString absolutePluginDir = QStringLiteral("%1/%2").arg(KEEPASSX_PREFIX_DIR, configuredPluginDir);
            pluginPaths << QDir(absolutePluginDir).canonicalPath();
        }
    }

    QStringList dirFilter;
    dirFilter << QStringLiteral("*%1*").arg(name);

    for (const QString& path : asConst(pluginPaths)) {
        const QStringList fileCandidates = QDir(path).entryList(dirFilter, QDir::Files);

        for (const QString& file : fileCandidates) {
            QString filePath = path + "/" + file;

            if (QLibrary::isLibrary(filePath)) {
                return filePath;
            }
        }
    }

    return {};
}

QString Resources::wordlistPath(const QString& name) const
{
    return dataPath(QStringLiteral("wordlists/%1").arg(name));
}

QString Resources::userWordlistPath(const QString& name) const
{
    QString configPath = QFileInfo(config()->getFileName()).absolutePath();
    return configPath + QStringLiteral("/wordlists/%1").arg(name);
}

Resources::Resources()
{
    const QString appDirPath = QCoreApplication::applicationDirPath();
#if defined(Q_OS_UNIX) && !(defined(Q_OS_MACOS) && defined(WITH_APP_BUNDLE))
    trySetResourceDir(KEEPASSX_DATA_DIR) || trySetResourceDir(QString("%1/../%2").arg(appDirPath, KEEPASSX_DATA_DIR))
        || trySetResourceDir(QString("%1/%2").arg(KEEPASSX_PREFIX_DIR, KEEPASSX_DATA_DIR));
#elif defined(Q_OS_MACOS) && defined(WITH_APP_BUNDLE)
    trySetResourceDir(appDirPath + QStringLiteral("/../Resources"));
#elif defined(Q_OS_WIN)
    trySetResourceDir(appDirPath + QStringLiteral("/share"));
#endif

    if (m_dataPath.isEmpty()) {
        // Last ditch check if we are running from inside the src or test build directory
        trySetResourceDir(appDirPath + QStringLiteral("/../share"))
            || trySetResourceDir(appDirPath + QStringLiteral("/../../share"));
    }

    if (m_dataPath.isEmpty()) {
        qWarning("Resources::DataPath: can't find data dir");
    }
}

bool Resources::trySetResourceDir(const QString& path)
{
    QDir dir(path);
    if (dir.exists()) {
        m_dataPath = dir.canonicalPath();
        return true;
    }
    return false;
}

Resources* Resources::instance()
{
    if (!m_instance) {
        m_instance = new Resources();
    }

    return m_instance;
}
