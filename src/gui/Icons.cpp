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

#include "Icons.h"

#include <QBitmap>
#include <QPainter>
#include <QStyle>

#include "config-keepassx.h"
#include "core/Config.h"
#include "gui/MainWindow.h"
#include "gui/osutils/OSUtils.h"

Icons* Icons::m_instance(nullptr);

QIcon Icons::applicationIcon()
{
    return icon("keepassxc", false);
}

QString Icons::trayIconAppearance() const
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

QIcon Icons::trayIcon()
{
    return trayIconUnlocked();
}

QIcon Icons::trayIconLocked()
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

QIcon Icons::trayIconUnlocked()
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

QIcon Icons::icon(const QString& name, bool recolor, const QColor& overrideColor)
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
        const QRect rect(0, 0, 48, 48);
        QImage img = icon.pixmap(rect.width(), rect.height()).toImage();
        img = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        icon = {};

        QPainter painter(&img);
        painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);

        if (!overrideColor.isValid()) {
            QPalette palette = getMainWindow()->palette();
            painter.fillRect(rect, palette.color(QPalette::Normal, QPalette::WindowText));
            icon.addPixmap(QPixmap::fromImage(img), QIcon::Normal);

            painter.fillRect(rect, palette.color(QPalette::Active, QPalette::ButtonText));
            icon.addPixmap(QPixmap::fromImage(img), QIcon::Active);

            painter.fillRect(rect, palette.color(QPalette::Active, QPalette::HighlightedText));
            icon.addPixmap(QPixmap::fromImage(img), QIcon::Selected);

            painter.fillRect(rect, palette.color(QPalette::Disabled, QPalette::WindowText));
            icon.addPixmap(QPixmap::fromImage(img), QIcon::Disabled);
        } else {
            painter.fillRect(rect, overrideColor);
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

QIcon Icons::onOffIcon(const QString& name, bool recolor)
{
    QString cacheName = "onoff/" + name;

    QIcon icon = m_iconCache.value(cacheName);

    if (!icon.isNull()) {
        return icon;
    }

    const QSize size(48, 48);
    QIcon on = Icons::icon(name + "-on", recolor);
    icon.addPixmap(on.pixmap(size, QIcon::Mode::Normal), QIcon::Mode::Normal, QIcon::On);
    icon.addPixmap(on.pixmap(size, QIcon::Mode::Selected), QIcon::Mode::Selected, QIcon::On);
    icon.addPixmap(on.pixmap(size, QIcon::Mode::Disabled), QIcon::Mode::Disabled, QIcon::On);

    QIcon off = Icons::icon(name + "-off", recolor);
    icon.addPixmap(off.pixmap(size, QIcon::Mode::Normal), QIcon::Mode::Normal, QIcon::Off);
    icon.addPixmap(off.pixmap(size, QIcon::Mode::Selected), QIcon::Mode::Selected, QIcon::Off);
    icon.addPixmap(off.pixmap(size, QIcon::Mode::Disabled), QIcon::Mode::Disabled, QIcon::Off);

    m_iconCache.insert(cacheName, icon);

    return icon;
}

Icons::Icons()
{
}

Icons* Icons::instance()
{
    if (!m_instance) {
        m_instance = new Icons();

        Q_INIT_RESOURCE(icons);
        QIcon::setThemeSearchPaths(QStringList{":/icons"} << QIcon::themeSearchPaths());
        QIcon::setThemeName("application");
    }

    return m_instance;
}
