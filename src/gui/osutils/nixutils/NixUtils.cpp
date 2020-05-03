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
#include <QGuiApplication>
#include <QPalette>
#include <QStyle>

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

bool NixUtils::isDarkMode()
{
    if (!qApp || !qApp->style()) {
        return false;
    }
    return qApp->style()->standardPalette().color(QPalette::Window).toHsl().lightness() < 110;
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
