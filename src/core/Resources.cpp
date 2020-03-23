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

#include <QBitmap>
#include <QDir>
#include <QLibrary>
#include <QStyle>

#include "config-keepassx.h"
#include "core/Config.h"
#include "core/Global.h"
#include "gui/MainWindow.h"

Resources* Resources::m_instance(nullptr);

QString Resources::dataPath(const QString& name)
{
    if (name.isEmpty() || name.startsWith('/')) {
        return m_dataPath + name;
    }
    return m_dataPath + "/" + name;
}

QString Resources::pluginPath(const QString& name)
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

QString Resources::wordlistPath(const QString& name)
{
    return dataPath(QStringLiteral("wordlists/%1").arg(name));
}

QIcon Resources::applicationIcon()
{
    return icon("keepassxc", false);
}

QIcon Resources::trayIcon()
{
    return useDarkIcon() ? icon("keepassxc-dark", false) : icon("keepassxc", false);
}

QIcon Resources::trayIconLocked()
{
    return icon("keepassxc-locked", false);
}

QIcon Resources::trayIconUnlocked()
{
    return useDarkIcon() ? icon("keepassxc-dark", false) : icon("keepassxc-unlocked", false);
}

QIcon Resources::icon(const QString& name, bool recolor)
{
    QIcon icon = m_iconCache.value(name);

    if (!icon.isNull()) {
        return icon;
    }

    icon = QIcon::fromTheme(name);
    if (getMainWindow() && recolor) {
        QPixmap pixmap = icon.pixmap(128, 128);
        icon = {};

        QPalette palette = getMainWindow()->palette();

        auto mask = QBitmap::fromImage(pixmap.toImage().createAlphaMask());
        pixmap.fill(palette.color(QPalette::WindowText));
        pixmap.setMask(mask);
        icon.addPixmap(pixmap, QIcon::Mode::Normal);

        pixmap.fill(palette.color(QPalette::HighlightedText));
        pixmap.setMask(mask);
        icon.addPixmap(pixmap, QIcon::Mode::Selected);

        pixmap.fill(palette.color(QPalette::Disabled, QPalette::WindowText));
        pixmap.setMask(mask);
        icon.addPixmap(pixmap, QIcon::Mode::Disabled);

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        icon.setIsMask(true);
#endif
    }

    m_iconCache.insert(name, icon);

    return icon;
}

QIcon Resources::onOffIcon(const QString& name, bool recolor)
{
    QString cacheName = "onoff/" + name;

    QIcon icon = m_iconCache.value(cacheName);

    if (!icon.isNull()) {
        return icon;
    }

    QIcon on = Resources::icon(name + "-on", recolor);
    for (const auto& size : on.availableSizes()) {
        icon.addPixmap(on.pixmap(size, QIcon::Mode::Normal), QIcon::Mode::Normal, QIcon::On);
        icon.addPixmap(on.pixmap(size, QIcon::Mode::Selected), QIcon::Mode::Selected, QIcon::On);
        icon.addPixmap(on.pixmap(size, QIcon::Mode::Disabled), QIcon::Mode::Disabled, QIcon::On);
    }
    QIcon off = Resources::icon(name + "-off", recolor);
    for (const auto& size : off.availableSizes()) {
        icon.addPixmap(off.pixmap(size, QIcon::Mode::Normal), QIcon::Mode::Normal, QIcon::Off);
        icon.addPixmap(off.pixmap(size, QIcon::Mode::Selected), QIcon::Mode::Selected, QIcon::Off);
        icon.addPixmap(off.pixmap(size, QIcon::Mode::Disabled), QIcon::Mode::Disabled, QIcon::Off);
    }

    m_iconCache.insert(cacheName, icon);

    return icon;
}

Resources::Resources()
{
    const QString appDirPath = QCoreApplication::applicationDirPath();
#if defined(Q_OS_UNIX) && !(defined(Q_OS_MACOS) && defined(WITH_APP_BUNDLE))
    testResourceDir(KEEPASSX_DATA_DIR) || testResourceDir(QStringLiteral("%1/../%2").arg(appDirPath, KEEPASSX_DATA_DIR))
        || testResourceDir(QStringLiteral("%1/%2").arg(KEEPASSX_PREFIX_DIR, KEEPASSX_DATA_DIR));
#elif defined(Q_OS_MACOS) && defined(WITH_APP_BUNDLE)
    testResourceDir(appDirPath + QStringLiteral("/../Resources"));
#elif defined(Q_OS_WIN)
    testResourceDir(appDirPath + QStringLiteral("/share"));
#endif

    if (m_dataPath.isEmpty()) {
        // Last ditch check if we are running from inside the src or test build directory
        testResourceDir(appDirPath + QStringLiteral("/../../share"))
            || testResourceDir(appDirPath + QStringLiteral("/../share"))
            || testResourceDir(appDirPath + QStringLiteral("/../../../share"));
    }

    if (m_dataPath.isEmpty()) {
        qWarning("Resources::DataPath: can't find data dir");
    }
}

bool Resources::testResourceDir(const QString& dir)
{
    if (QFile::exists(dir + QStringLiteral("/icons/application/256x256/apps/keepassxc.png"))) {
        m_dataPath = QDir::cleanPath(dir);
        return true;
    }
    return false;
}

bool Resources::useDarkIcon()
{
    return config()->get("GUI/DarkTrayIcon").toBool();
}

Resources* Resources::instance()
{
    if (!m_instance) {
        m_instance = new Resources();

        Q_INIT_RESOURCE(icons);
        QIcon::setThemeSearchPaths(QStringList{":/icons"} << QIcon::themeSearchPaths());
        QIcon::setThemeName("application");
    }

    return m_instance;
}
