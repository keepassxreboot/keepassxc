/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#include "DarkStyle.h"
#include "gui/osutils/OSUtils.h"

#include <QDialog>
#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>

void DarkStyle::polish(QPalette& palette)
{
    palette.setColor(QPalette::Active, QPalette::Window, QStringLiteral("#3B3B3D"));
    palette.setColor(QPalette::Inactive, QPalette::Window, QStringLiteral("#404042"));
    palette.setColor(QPalette::Disabled, QPalette::Window, QStringLiteral("#424242"));

    palette.setColor(QPalette::Active, QPalette::WindowText, QStringLiteral("#CACBCE"));
    palette.setColor(QPalette::Inactive, QPalette::WindowText, QStringLiteral("#C8C8C6"));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QStringLiteral("#707070"));

    palette.setColor(QPalette::Active, QPalette::Text, QStringLiteral("#CACBCE"));
    palette.setColor(QPalette::Inactive, QPalette::Text, QStringLiteral("#C8C8C6"));
    palette.setColor(QPalette::Disabled, QPalette::Text, QStringLiteral("#707070"));

#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    palette.setColor(QPalette::Active, QPalette::PlaceholderText, QStringLiteral("#7D7D82"));
    palette.setColor(QPalette::Inactive, QPalette::PlaceholderText, QStringLiteral("#87888C"));
    palette.setColor(QPalette::Disabled, QPalette::PlaceholderText, QStringLiteral("#737373"));
#endif

    palette.setColor(QPalette::Active, QPalette::BrightText, QStringLiteral("#252627"));
    palette.setColor(QPalette::Inactive, QPalette::BrightText, QStringLiteral("#2D2D2F"));
    palette.setColor(QPalette::Disabled, QPalette::BrightText, QStringLiteral("#333333"));

    palette.setColor(QPalette::Active, QPalette::Base, QStringLiteral("#27272A"));
    palette.setColor(QPalette::Inactive, QPalette::Base, QStringLiteral("#2A2A2D"));
    palette.setColor(QPalette::Disabled, QPalette::Base, QStringLiteral("#343437"));

    palette.setColor(QPalette::Active, QPalette::AlternateBase, QStringLiteral("#303036"));
    palette.setColor(QPalette::Inactive, QPalette::AlternateBase, QStringLiteral("#333338"));
    palette.setColor(QPalette::Disabled, QPalette::AlternateBase, QStringLiteral("#36363A"));

    palette.setColor(QPalette::All, QPalette::ToolTipBase, QStringLiteral("#2D532D"));
    palette.setColor(QPalette::All, QPalette::ToolTipText, QStringLiteral("#BFBFBF"));

    palette.setColor(QPalette::Active, QPalette::Button, QStringLiteral("#28282B"));
    palette.setColor(QPalette::Inactive, QPalette::Button, QStringLiteral("#2B2B2E"));
    palette.setColor(QPalette::Disabled, QPalette::Button, QStringLiteral("#2B2A2A"));

    palette.setColor(QPalette::Active, QPalette::ButtonText, QStringLiteral("#B9B9BE"));
    palette.setColor(QPalette::Inactive, QPalette::ButtonText, QStringLiteral("#9E9FA5"));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QStringLiteral("#73747E"));

    palette.setColor(QPalette::Active, QPalette::Highlight, QStringLiteral("#2D532D"));
    palette.setColor(QPalette::Inactive, QPalette::Highlight, QStringLiteral("#294C29"));
    palette.setColor(QPalette::Disabled, QPalette::Highlight, QStringLiteral("#293D29"));

    palette.setColor(QPalette::Active, QPalette::HighlightedText, QStringLiteral("#CCCCCC"));
    palette.setColor(QPalette::Inactive, QPalette::HighlightedText, QStringLiteral("#C7C7C7"));
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QStringLiteral("#707070"));

    palette.setColor(QPalette::All, QPalette::Light, QStringLiteral("#414145"));
    palette.setColor(QPalette::All, QPalette::Midlight, QStringLiteral("#39393C"));
    palette.setColor(QPalette::All, QPalette::Mid, QStringLiteral("#2F2F32"));
    palette.setColor(QPalette::All, QPalette::Dark, QStringLiteral("#202022"));
    palette.setColor(QPalette::All, QPalette::Shadow, QStringLiteral("#19191A"));

    palette.setColor(QPalette::All, QPalette::Link, QStringLiteral("#6BAE6B"));
    palette.setColor(QPalette::Disabled, QPalette::Link, QStringLiteral("#9DE9D"));
    palette.setColor(QPalette::All, QPalette::LinkVisited, QStringLiteral("#70A970"));
    palette.setColor(QPalette::Disabled, QPalette::LinkVisited, QStringLiteral("#98A998"));
}

QString DarkStyle::getAppStyleSheet() const
{
    QFile extStylesheetFile(QStringLiteral(":/styles/dark/darkstyle.qss"));
    if (extStylesheetFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return extStylesheetFile.readAll();
    }
    qWarning("Failed to load dark theme stylesheet.");
    return {};
}

void DarkStyle::polish(QWidget* widget)
{
    if (qobject_cast<QMainWindow*>(widget) || qobject_cast<QDialog*>(widget) || qobject_cast<QMenuBar*>(widget)
        || qobject_cast<QToolBar*>(widget)) {
        auto palette = widget->palette();
#if defined(Q_OS_MACOS)
        if (osUtils->isDarkMode()) {
            // Let the Cocoa platform plugin draw its own background
            palette.setColor(QPalette::All, QPalette::Window, Qt::transparent);
        } else {
            palette.setColor(QPalette::Active, QPalette::Window, QStringLiteral("#2A2A2A"));
            palette.setColor(QPalette::Inactive, QPalette::Window, QStringLiteral("#2D2D2D"));
            palette.setColor(QPalette::Disabled, QPalette::Window, QStringLiteral("#2A2A2A"));
        }
#elif defined(Q_OS_WIN)
        // Register event filter for better dark mode support
        WinUtils::registerEventFilters();
        palette.setColor(QPalette::All, QPalette::Window, QStringLiteral("#2F2F30"));
#else
        palette.setColor(QPalette::Active, QPalette::Window, QStringLiteral("#2F2F30"));
        palette.setColor(QPalette::Inactive, QPalette::Window, QStringLiteral("#313133"));
        palette.setColor(QPalette::Disabled, QPalette::Window, QStringLiteral("#3A3A3B"));
#endif

        widget->setPalette(palette);
    }
}
