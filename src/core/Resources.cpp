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
#include <QPainter>
#include <QStyle>

#include "config-keepassx.h"
#include "core/Config.h"
#include "core/Global.h"
#include "gui/MainWindow.h"
#include "gui/osutils/OSUtils.h"

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

QIcon Resources::applicationIcon()
{
    return icon("keepassxc", false);
}

QString Resources::trayIconAppearance() const
{
    auto iconAppearance = config()->get(Config::GUI_TrayIconAppearance).toString();
    if (iconAppearance.isNull()) {
#ifdef Q_OS_MACOS
        iconAppearance = osUtils->isDarkMode() ? "monochrome-light" : "monochrome-dark";
#else
        iconAppearance = "monochrome-light";
#endif
    }
    return iconAppearance;
}

QIcon Resources::trayIcon()
{
    return trayIconUnlocked();
}

QIcon Resources::trayIconLocked()
{
    auto iconApperance = trayIconAppearance();

    if (iconApperance == "monochrome-light") {
        return icon("keepassxc-monochrome-light-locked", false);
    }
    if (iconApperance == "monochrome-dark") {
        return icon("keepassxc-monochrome-dark-locked", false);
    }
    return icon("keepassxc-locked", false);
}

QIcon Resources::trayIconUnlocked()
{
    auto iconApperance = trayIconAppearance();

    if (iconApperance == "monochrome-light") {
        return icon("keepassxc-monochrome-light", false);
    }
    if (iconApperance == "monochrome-dark") {
        return icon("keepassxc-monochrome-dark", false);
    }
    return icon("keepassxc", false);
}

QIcon Resources::icon(const QString& name, bool recolor, const QColor& overrideColor)
{
    QIcon icon = m_iconCache.value(name);

    if (!icon.isNull() && !overrideColor.isValid()) {
        return icon;
    }

    // Resetting the application theme name before calling QIcon::fromTheme() is required for hacky
    // QPA platform themes such as qt5ct, which randomly mess with the configured icon theme.
    // If we do not reset the theme name here, it will become empty at some point, causing
    // Qt to look for icons at the user-level and global default locations.
    //
    // See issue #4963: https://github.com/keepassxreboot/keepassxc/issues/4963
    // and qt5ct issue #80: https://sourceforge.net/p/qt5ct/tickets/80/
    QIcon::setThemeName("application");

    icon = QIcon::fromTheme(name);
    if (getMainWindow() && recolor) {
        QImage img = icon.pixmap(128, 128).toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
        icon = {};

        QPainter painter(&img);
        painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);

        if (!overrideColor.isValid()) {
            QPalette palette = getMainWindow()->palette();
            painter.fillRect(0, 0, img.width(), img.height(), palette.color(QPalette::Normal, QPalette::WindowText));
            icon.addPixmap(QPixmap::fromImage(img), QIcon::Normal);

            painter.fillRect(0, 0, img.width(), img.height(), palette.color(QPalette::Active, QPalette::ButtonText));
            icon.addPixmap(QPixmap::fromImage(img), QIcon::Active);

            painter.fillRect(
                0, 0, img.width(), img.height(), palette.color(QPalette::Active, QPalette::HighlightedText));
            icon.addPixmap(QPixmap::fromImage(img), QIcon::Selected);

            painter.fillRect(0, 0, img.width(), img.height(), palette.color(QPalette::Disabled, QPalette::WindowText));
            icon.addPixmap(QPixmap::fromImage(img), QIcon::Disabled);
        } else {
            painter.fillRect(0, 0, img.width(), img.height(), overrideColor);
            icon.addPixmap(QPixmap::fromImage(img), QIcon::Normal);
        }

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        icon.setIsMask(true);
#endif
    }

    if (!overrideColor.isValid()) {
        m_iconCache.insert(name, icon);
    }

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

        Q_INIT_RESOURCE(icons);
        QIcon::setThemeSearchPaths(QStringList{":/icons"} << QIcon::themeSearchPaths());
        QIcon::setThemeName("application");
    }

    return m_instance;
}
