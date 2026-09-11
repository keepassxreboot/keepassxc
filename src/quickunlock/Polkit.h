/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#pragma once

#include "PinUnlock.h"
#include "polkit_dbus.h"

#include <QHash>
#include <QScopedPointer>

class Polkit : public PinUnlock
{
public:
    Polkit();
    ~Polkit() override;

    bool isAvailable() const override;

    bool setKey(const QUuid& dbUuid, const QByteArray& data) override;
    bool getKey(const QUuid& dbUuid, QByteArray& data) override;
    bool hasKey(const QUuid& dbUuid) const override;

    void reset(const QUuid& dbUuid) override;
    void reset() override;

private:
    bool promptPolkit();

    bool m_available;
    QHash<QUuid, QByteArray> m_sessionKeys;

    QScopedPointer<org::freedesktop::PolicyKit1::Authority> m_polkit;
};
