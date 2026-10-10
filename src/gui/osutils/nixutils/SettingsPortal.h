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

#ifndef KEEPASSXC_SETTINGSPORTAL_H
#define KEEPASSXC_SETTINGSPORTAL_H

#include <QObject>
#include <QVariant>

class SettingsPortal : public QObject
{
    Q_OBJECT

public:
    // "org.freedesktop.appearance color-scheme" as specified in
    // gui/osutils/nixutils/dbus/org.freedesktop.portal.Settings.xml
    enum ColorschemePref
    {
        PreferNone,
        PreferDark,
        PreferLight
    };

    explicit SettingsPortal(QObject* parent = nullptr);

    bool isAvailable() const;
    bool hasColorScheme() const;
    ColorschemePref colorScheme() const;

signals:
    void colorSchemeChanged(ColorschemePref pref);

private:
    void setColorScheme(const QVariant& value);

    ColorschemePref m_pref = ColorschemePref::PreferNone;
    bool m_prefExists = false;
};

#endif // KEEPASSXC_SETTINGSPORTAL_H
