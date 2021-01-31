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

#include "WinUtils.h"
#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QDir>
#include <QSettings>

#include <windows.h>

QPointer<WinUtils> WinUtils::m_instance = nullptr;
QScopedPointer<WinUtils::DWMEventFilter> WinUtils::m_eventFilter;

WinUtils* WinUtils::instance()
{
    if (!m_instance) {
        m_instance = new WinUtils(qApp);
        m_instance->m_darkAppThemeActive = m_instance->isDarkMode();
        m_instance->m_darkSystemThemeActive = m_instance->isStatusBarDark();
    }

    return m_instance;
}

WinUtils::WinUtils(QObject* parent)
    : OSUtilsBase(parent)
{
}

WinUtils::~WinUtils()
{
}

/**
 * Register event filters to handle native platform events such as theme changes.
 */
void WinUtils::registerEventFilters()
{
    if (!m_eventFilter) {
        m_eventFilter.reset(new DWMEventFilter);
        qApp->installNativeEventFilter(m_eventFilter.data());
    }
}

bool WinUtils::DWMEventFilter::nativeEventFilter(const QByteArray& eventType, void* message, long*)
{
    if (eventType != "windows_generic_MSG") {
        return false;
    }

    auto* msg = static_cast<MSG*>(message);
    if (!msg->hwnd) {
        return false;
    }
    switch (msg->message) {
    case WM_SETTINGCHANGE:
        if (m_instance->m_darkAppThemeActive != m_instance->isDarkMode()) {
            m_instance->m_darkAppThemeActive = !m_instance->m_darkAppThemeActive;
            emit m_instance->interfaceThemeChanged();
        }

        if (m_instance->m_darkSystemThemeActive != m_instance->isStatusBarDark()) {
            m_instance->m_darkSystemThemeActive = !m_instance->m_darkSystemThemeActive;
            emit m_instance->statusbarThemeChanged();
        }
        break;
    }

    return false;
}

bool WinUtils::isDarkMode() const
{
    QSettings settings(R"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)",
                       QSettings::NativeFormat);
    return settings.value("AppsUseLightTheme", 1).toInt() == 0;
}

bool WinUtils::isStatusBarDark() const
{
    QSettings settings(R"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)",
                       QSettings::NativeFormat);
    return settings.value("SystemUsesLightTheme", 0).toInt() == 0;
}

bool WinUtils::isLaunchAtStartupEnabled() const
{
    return QSettings(R"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run)", QSettings::NativeFormat)
        .contains(qAppName());
}

void WinUtils::setLaunchAtStartup(bool enable)
{
    QSettings reg(R"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run)", QSettings::NativeFormat);
    if (enable) {
        reg.setValue(qAppName(), QString("\"%1\"").arg(QDir::toNativeSeparators(QApplication::applicationFilePath())));
    } else {
        reg.remove(qAppName());
    }
}

bool WinUtils::isCapslockEnabled()
{
    return GetKeyState(VK_CAPITAL) == 1;
}

bool WinUtils::isHighContrastMode() const
{
    QSettings settings(R"(HKEY_CURRENT_USER\Control Panel\Accessibility\HighContrast)", QSettings::NativeFormat);
    return (settings.value("Flags").toInt() & 1u) != 0;
}
