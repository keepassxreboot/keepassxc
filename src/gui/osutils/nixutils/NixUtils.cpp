/*
 * Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or (at your option)
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "NixUtils.h"

#include <QApplication>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QPalette>
#include <QStandardPaths>
#include <QStyle>
#include <QTextStream>

#include <qpa/qplatformnativeinterface.h>
// namespace required to avoid name clashes with declarations in XKBlib.h
namespace X11
{
#include <X11/XKBlib.h>
}

QPointer<NixUtils> NixUtils::m_instance = nullptr;

NixUtils* NixUtils::instance()
{
    if (!m_instance) {
        m_instance = new NixUtils(qApp);
    }

    return m_instance;
}

NixUtils::NixUtils(QObject* parent)
    : OSUtilsBase(parent)
{
}

NixUtils::~NixUtils()
{
}

bool NixUtils::isDarkMode() const
{
    if (!qApp || !qApp->style()) {
        return false;
    }
    return qApp->style()->standardPalette().color(QPalette::Window).toHsl().lightness() < 110;
}

QString NixUtils::getAutostartDesktopFilename(bool createDirs) const
{
    QDir autostartDir;
    auto confHome = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    if (confHome.isEmpty()) {
        return {};
    }
    autostartDir.setPath(confHome + QStringLiteral("/autostart"));
    if (createDirs && !autostartDir.exists()) {
        autostartDir.mkpath(".");
    }

    return QFile(autostartDir.absoluteFilePath(qApp->property("KPXC_QUALIFIED_APPNAME").toString().append(".desktop")))
        .fileName();
}

bool NixUtils::isLaunchAtStartupEnabled() const
{
    return QFile::exists(getAutostartDesktopFilename());
    ;
}

void NixUtils::setLaunchAtStartup(bool enable)
{
    if (enable) {
        QFile desktopFile(getAutostartDesktopFilename(true));
        if (!desktopFile.open(QIODevice::WriteOnly)) {
            qWarning("Failed to create autostart desktop file.");
            return;
        }
        QTextStream stream(&desktopFile);
        stream.setCodec("UTF-8");
        stream << QStringLiteral("[Desktop Entry]") << '\n'
               << QStringLiteral("Name=") << QApplication::applicationDisplayName() << '\n'
               << QStringLiteral("GenericName=") << tr("Password Manager") << '\n'
               << QStringLiteral("Exec=") << QApplication::applicationFilePath() << '\n'
               << QStringLiteral("TryExec=") << QApplication::applicationFilePath() << '\n'
               << QStringLiteral("Icon=") << QApplication::applicationName().toLower() << '\n'
               << QStringLiteral("StartupWMClass=keepassxc") << '\n'
               << QStringLiteral("StartupNotify=true") << '\n'
               << QStringLiteral("Terminal=false") << '\n'
               << QStringLiteral("Type=Application") << '\n'
               << QStringLiteral("Version=1.0") << "true" << '\n'
               << QStringLiteral("Categories=Utility;Security;Qt;") << '\n'
               << QStringLiteral("MimeType=application/x-keepass2;") << '\n'
               << QStringLiteral("X-GNOME-Autostart-enabled=true") << endl;
        desktopFile.close();
    } else if (isLaunchAtStartupEnabled()) {
        QFile::remove(getAutostartDesktopFilename());
    }
}

bool NixUtils::isCapslockEnabled()
{
    QPlatformNativeInterface* native = QGuiApplication::platformNativeInterface();
    auto* display = native->nativeResourceForWindow("display", nullptr);
    if (!display) {
        return false;
    }

    QString platform = QGuiApplication::platformName();
    if (platform == "xcb") {
        unsigned state = 0;
        if (X11::XkbGetIndicatorState(reinterpret_cast<X11::Display*>(display), XkbUseCoreKbd, &state) == Success) {
            return ((state & 1u) != 0);
        }
    }

    // TODO: Wayland

    return false;
}
