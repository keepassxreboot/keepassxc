/*
 * Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "SettingsPortal.h"

#include "xdp_settings.h"

#include <QDBusConnection>

Q_GLOBAL_STATIC_WITH_ARGS(OrgFreedesktopPortalSettingsInterface,
                          s_settingsInterface,
                          ("org.freedesktop.portal.Desktop",
                           "/org/freedesktop/portal/desktop",
                           QDBusConnection::sessionBus()));

namespace
{
    const auto kColorSchemeNamespace = QLatin1String("org.freedesktop.appearance");
    const auto kColorSchemeKey = QLatin1String("color-scheme");
} // namespace

SettingsPortal::SettingsPortal(QObject* parent)
    : QObject(parent)
{
    if (!isAvailable()) {
        return;
    }

    connect(s_settingsInterface,
            &OrgFreedesktopPortalSettingsInterface::SettingChanged,
            this,
            [this](const QString& ns, const QString& key, const QDBusVariant& value) {
                if (ns == kColorSchemeNamespace && key == kColorSchemeKey) {
                    setColorScheme(value.variant());
                }
            });

    auto reply = s_settingsInterface->ReadOne(kColorSchemeNamespace, kColorSchemeKey);
    reply.waitForFinished();
    if (!reply.isError()) {
        setColorScheme(reply.value().variant());
    }
}

bool SettingsPortal::isAvailable() const
{
    return s_settingsInterface->version() > 0;
}

bool SettingsPortal::hasColorScheme() const
{
    return m_prefExists;
}

SettingsPortal::ColorschemePref SettingsPortal::colorScheme() const
{
    return m_pref;
}

void SettingsPortal::setColorScheme(const QVariant& value)
{
    m_pref = static_cast<ColorschemePref>(value.toInt());
    m_prefExists = true;
    emit colorSchemeChanged(m_pref);
}
