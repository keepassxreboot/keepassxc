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

#include "LightStyle.h"
#include "gui/ApplicationSettingsWidget.h"
#include "gui/osutils/OSUtils.h"

#include <QDialog>
#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>

void LightStyle::polish(QPalette& palette)
{
    palette.setColor(QPalette::Active, QPalette::Window, QStringLiteral("#F7F7F7"));
    palette.setColor(QPalette::Inactive, QPalette::Window, QStringLiteral("#FCFCFC"));
    palette.setColor(QPalette::Disabled, QPalette::Window, QStringLiteral("#EDEDED"));

    palette.setColor(QPalette::Active, QPalette::WindowText, QStringLiteral("#1D1D20"));
    palette.setColor(QPalette::Inactive, QPalette::WindowText, QStringLiteral("#252528"));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QStringLiteral("#8C8C92"));

    palette.setColor(QPalette::Active, QPalette::Text, QStringLiteral("#1D1D20"));
    palette.setColor(QPalette::Inactive, QPalette::Text, QStringLiteral("#252528"));
    palette.setColor(QPalette::Disabled, QPalette::Text, QStringLiteral("#8C8C92"));

#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    palette.setColor(QPalette::Active, QPalette::PlaceholderText, QStringLiteral("#71727D"));
    palette.setColor(QPalette::Inactive, QPalette::PlaceholderText, QStringLiteral("#878893"));
    palette.setColor(QPalette::Disabled, QPalette::PlaceholderText, QStringLiteral("#A3A4AC"));
#endif

    palette.setColor(QPalette::Active, QPalette::BrightText, QStringLiteral("#F3F3F4"));
    palette.setColor(QPalette::Inactive, QPalette::BrightText, QStringLiteral("#EAEAEB"));
    palette.setColor(QPalette::Disabled, QPalette::BrightText, QStringLiteral("#E4E5E7"));

    palette.setColor(QPalette::Active, QPalette::Base, QStringLiteral("#F9F9F9"));
    palette.setColor(QPalette::Inactive, QPalette::Base, QStringLiteral("#F5F5F4"));
    palette.setColor(QPalette::Disabled, QPalette::Base, QStringLiteral("#EFEFF2"));

    palette.setColor(QPalette::Active, QPalette::AlternateBase, QStringLiteral("#ECF3E8"));
    palette.setColor(QPalette::Inactive, QPalette::AlternateBase, QStringLiteral("#EAF2E6"));
    palette.setColor(QPalette::Disabled, QPalette::AlternateBase, QStringLiteral("#E1E9DD"));

    palette.setColor(QPalette::All, QPalette::ToolTipBase, QStringLiteral("#548C1D"));
    palette.setColor(QPalette::All, QPalette::ToolTipText, QStringLiteral("#F7F7F7"));

    palette.setColor(QPalette::Active, QPalette::Button, QStringLiteral("#D4D5DD"));
    palette.setColor(QPalette::Inactive, QPalette::Button, QStringLiteral("#DCDCE0"));
    palette.setColor(QPalette::Disabled, QPalette::Button, QStringLiteral("#E5E5E6"));

    palette.setColor(QPalette::Active, QPalette::ButtonText, QStringLiteral("#181A18"));
    palette.setColor(QPalette::Inactive, QPalette::ButtonText, QStringLiteral("#5F6671"));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QStringLiteral("#97979B"));

    palette.setColor(QPalette::Active, QPalette::Highlight, QStringLiteral("#549712"));
    palette.setColor(QPalette::Inactive, QPalette::Highlight, QStringLiteral("#528D16"));
    palette.setColor(QPalette::Disabled, QPalette::Highlight, QStringLiteral("#6F9847"));

    palette.setColor(QPalette::Active, QPalette::HighlightedText, QStringLiteral("#FCFCFC"));
    palette.setColor(QPalette::Inactive, QPalette::HighlightedText, QStringLiteral("#F2F2F2"));
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QStringLiteral("#D9D9D9"));

    palette.setColor(QPalette::All, QPalette::Light, QStringLiteral("#F9F9F9"));
    palette.setColor(QPalette::All, QPalette::Midlight, QStringLiteral("#E9E9EB"));
    palette.setColor(QPalette::All, QPalette::Mid, QStringLiteral("#C9C9CF"));
    palette.setColor(QPalette::All, QPalette::Dark, QStringLiteral("#BBBBC2"));
    palette.setColor(QPalette::All, QPalette::Shadow, QStringLiteral("#6C6D79"));

    palette.setColor(QPalette::All, QPalette::Link, QStringLiteral("#429F14"));
    palette.setColor(QPalette::Disabled, QPalette::Link, QStringLiteral("#949F8F"));
    palette.setColor(QPalette::All, QPalette::LinkVisited, QStringLiteral("#3F8C17"));
    palette.setColor(QPalette::Disabled, QPalette::LinkVisited, QStringLiteral("#838C7E"));
}

QString LightStyle::getAppStyleSheet() const
{
    QFile extStylesheetFile(QStringLiteral(":/styles/light/lightstyle.qss"));
    if (extStylesheetFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return extStylesheetFile.readAll();
    }
    qWarning("Failed to load light theme stylesheet.");
    return {};
}

void LightStyle::polish(QWidget* widget)
{
    if (qobject_cast<QMainWindow*>(widget) || qobject_cast<QDialog*>(widget) || qobject_cast<QMenuBar*>(widget)
        || qobject_cast<QToolBar*>(widget)) {
        auto palette = widget->palette();
#if defined(Q_OS_MACOS)
        if (!osUtils->isDarkMode()) {
            // Let the Cocoa platform plugin draw its own background
            palette.setColor(QPalette::All, QPalette::Window, Qt::transparent);
        } else {
            palette.setColor(QPalette::Active, QPalette::Window, QStringLiteral("#D6D6D6"));
            palette.setColor(QPalette::Inactive, QPalette::Window, QStringLiteral("#F6F6F6"));
            palette.setColor(QPalette::Disabled, QPalette::Window, QStringLiteral("#D4D4D4"));
        }
#elif defined(Q_OS_WIN)
        palette.setColor(QPalette::All, QPalette::Window, QStringLiteral("#FFFFFF"));
#else
        palette.setColor(QPalette::Active, QPalette::Window, QStringLiteral("#EFF0F1"));
        palette.setColor(QPalette::Inactive, QPalette::Window, QStringLiteral("#EFF0F1"));
        palette.setColor(QPalette::Disabled, QPalette::Window, QStringLiteral("#E1E2E4"));
#endif

        widget->setPalette(palette);
    }
}
