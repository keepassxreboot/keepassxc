/*
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "FilePath.h"

#include <QCoreApplication>
#include <QDir>
#include <QLibrary>

#include "config-keepassx.h"
#include "core/Global.h"
#include "core/Config.h"

FilePath* FilePath::m_instance(nullptr);

QString FilePath::dataPath(const QString& name)
{
    if (name.isEmpty() || name.startsWith('/')) {
        return m_dataPath + name;
    }
    else {
        return m_dataPath + "/" + name;
    }
}

QString FilePath::pluginPath(const QString& name)
{
    QStringList pluginPaths;

    QDir buildDir(QCoreApplication::applicationDirPath() + "/autotype");
    const QStringList buildDirEntryList = buildDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& dir : buildDirEntryList) {
        pluginPaths << QCoreApplication::applicationDirPath() + "/autotype/" + dir;
    }

    // for TestAutoType
    pluginPaths << QCoreApplication::applicationDirPath() + "/../src/autotype/test";

#if defined(Q_OS_MAC) && defined(WITH_APP_BUNDLE)
    pluginPaths << QCoreApplication::applicationDirPath() + "/../PlugIns";
#endif

    pluginPaths << QCoreApplication::applicationDirPath();

    QString configuredPluginDir = KEEPASSX_PLUGIN_DIR;
    if (configuredPluginDir != ".") {
        if (QDir(configuredPluginDir).isAbsolute()) {
            pluginPaths << configuredPluginDir;
        }
        else {
            QString relativePluginDir = QString("%1/../%2")
                    .arg(QCoreApplication::applicationDirPath(), configuredPluginDir);
            pluginPaths << QDir(relativePluginDir).canonicalPath();

            QString absolutePluginDir = QString("%1/%2")
                    .arg(KEEPASSX_PREFIX_DIR, configuredPluginDir);
            pluginPaths << QDir(absolutePluginDir).canonicalPath();
        }
    }

    QStringList dirFilter;
    dirFilter << QString("*%1*").arg(name);

    for (const QString& path : asConst(pluginPaths)) {
        const QStringList fileCandidates = QDir(path).entryList(dirFilter, QDir::Files);

        for (const QString& file : fileCandidates) {
            QString filePath = path + "/" + file;

            if (QLibrary::isLibrary(filePath)) {
                return filePath;
            }
        }
    }

    return QString();
}

QString FilePath::wordlistPath(const QString& name)
{
    return dataPath("wordlists/" + name);
}

QIcon FilePath::applicationIcon()
{
#ifdef KEEPASSXC_DIST_SNAP
    return icon("apps", "keepassxc", false);
#else
    return icon("apps", "keepassxc");
#endif
}

QIcon FilePath::trayIcon()
{
    bool darkIcon = useDarkIcon();

#ifdef KEEPASSXC_DIST_SNAP
    return (darkIcon) ? icon("apps", "keepassxc-dark", false) : icon("apps", "keepassxc", false);
#else
    return (darkIcon) ? icon("apps", "keepassxc-dark") : icon("apps", "keepassxc");
#endif
}

QIcon FilePath::trayIconLocked()
{
#ifdef KEEPASSXC_DIST_SNAP
    return icon("apps", "keepassxc-locked", false);
#else
    return icon("apps", "keepassxc-locked");
#endif
}

QIcon FilePath::trayIconUnlocked()
{
    bool darkIcon = useDarkIcon();

#ifdef KEEPASSXC_DIST_SNAP
    return darkIcon ? icon("apps", "keepassxc-dark", false) : icon("apps", "keepassxc-unlocked", false);
#else
    return darkIcon ? icon("apps", "keepassxc-dark") : icon("apps", "keepassxc-unlocked");
#endif
}

QIcon FilePath::icon(const QString& category, const QString& name, bool fromTheme)
{
    QString combinedName = category + "/" + name;

    QIcon icon = m_iconCache.value(combinedName);

    if (!icon.isNull()) {
        return icon;
    }

    if (fromTheme) {
        icon = QIcon::fromTheme(name);
    }

    if (icon.isNull()) {
        const QList<int> pngSizes = { 16, 22, 24, 32, 48, 64, 128 };
        QString filename;
        for (int size : pngSizes) {
            filename = QString("%1/icons/application/%2x%2/%3.png").arg(m_dataPath, QString::number(size),
                                                                        combinedName);
            if (QFile::exists(filename)) {
                icon.addFile(filename, QSize(size, size));
            }
        }
        filename = QString("%1/icons/application/scalable/%2.svgz").arg(m_dataPath, combinedName);
        if (QFile::exists(filename)) {
            icon.addFile(filename);
        }
    }

    m_iconCache.insert(combinedName, icon);

    return icon;
}

QIcon FilePath::onOffIcon(const QString& category, const QString& name)
{
    QString combinedName = category + "/" + name;
    QString cacheName = "onoff/" + combinedName;

    QIcon icon = m_iconCache.value(cacheName);

    if (!icon.isNull()) {
        return icon;
    }

    for (int i = 0; i < 2; i++) {
        QIcon::State state;
        QString stateName;

        if (i == 0) {
            state = QIcon::Off;
            stateName = "off";
        }
        else {
            state = QIcon::On;
            stateName = "on";
        }

        const QList<int> pngSizes = { 16, 22, 24, 32, 48, 64, 128 };
        QString filename;
        for (int size : pngSizes) {
            filename = QString("%1/icons/application/%2x%2/%3-%4.png").arg(m_dataPath, QString::number(size),
                                                                           combinedName, stateName);
            if (QFile::exists(filename)) {
                icon.addFile(filename, QSize(size, size), QIcon::Normal, state);
            }
        }
        filename = QString("%1/icons/application/scalable/%2-%3.svgz").arg(m_dataPath, combinedName, stateName);
        if (QFile::exists(filename)) {
            icon.addFile(filename, QSize(), QIcon::Normal, state);
        }
    }

    m_iconCache.insert(cacheName, icon);

    return icon;
}

FilePath::FilePath()
{
    const QString appDirPath = QCoreApplication::applicationDirPath();
    bool isDataDirAbsolute = QDir::isAbsolutePath(KEEPASSX_DATA_DIR);
    Q_UNUSED(isDataDirAbsolute);

    if (false) {
    }
#ifdef QT_DEBUG
    else if (testSetDir(QString(KEEPASSX_SOURCE_DIR) + "/share")) {
    }
#endif
#if defined(Q_OS_UNIX) && !(defined(Q_OS_MAC) && defined(WITH_APP_BUNDLE))
    else if (isDataDirAbsolute && testSetDir(KEEPASSX_DATA_DIR)) {
    }
    else if (!isDataDirAbsolute && testSetDir(QString("%1/../%2").arg(appDirPath, KEEPASSX_DATA_DIR))) {
    }
    else if (!isDataDirAbsolute && testSetDir(QString("%1/%2").arg(KEEPASSX_PREFIX_DIR, KEEPASSX_DATA_DIR))) {
    }
#endif
#if defined(Q_OS_MAC) && defined(WITH_APP_BUNDLE)
    else if (testSetDir(appDirPath + "/../Resources")) {
    }
#endif
#ifdef Q_OS_WIN
    else if (testSetDir(appDirPath + "/share")) {
    }
#endif
    // Last ditch test when running in the build directory (mainly for travis tests)
    else if (testSetDir(QString(KEEPASSX_SOURCE_DIR) + "/share")) {
    }

    if (m_dataPath.isEmpty()) {
        qWarning("FilePath::DataPath: can't find data dir");
    }
    else {
        m_dataPath = QDir::cleanPath(m_dataPath);
    }
}

bool FilePath::testSetDir(const QString& dir)
{
    if (QFile::exists(dir + "/icons/database/C00_Password.png")) {
        m_dataPath = dir;
        return true;
    }
    else {
        return false;
    }
}

bool FilePath::useDarkIcon()
{
    return config()->get("GUI/DarkTrayIcon").toBool();
}

FilePath* FilePath::instance()
{
    if (!m_instance) {
        m_instance = new FilePath();
    }

    return m_instance;
}
