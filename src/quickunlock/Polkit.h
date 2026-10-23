/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_POLKIT_H
#define KEEPASSXC_POLKIT_H

#include "QuickUnlockInterface.h"
#include "polkit_dbus.h"
#include <QHash>
#include <QScopedPointer>
#include <QSharedPointer>

class SecretMemory;

class Polkit : public QuickUnlockInterface
{
public:
    Polkit();
    ~Polkit() override;

    bool isAvailable() const override;
    QString errorString() const override;

    bool setKey(const QUuid& dbUuid, const QByteArray& key) override;
    bool getKey(const QUuid& dbUuid, QByteArray& key) override;
    bool hasKey(const QUuid& dbUuid) const override;

    void reset(const QUuid& dbUuid) override;
    void reset() override;

private:
    bool storeSecret(const QUuid& dbUuid, const QByteArray& secret);
    bool retrieveSecret(const QUuid& dbUuid, QByteArray& secret);
    bool hasSecret(const QUuid& dbUuid) const;

    bool m_available;
    QString m_error;
    QHash<QUuid, QByteArray> m_encryptedMasterKeys;
    // Fallback storage for when the Linux keyring is unavailable (e.g. inside a Flatpak sandbox)
    QHash<QUuid, QSharedPointer<SecretMemory>> m_secretMemory;

    QScopedPointer<org::freedesktop::PolicyKit1::Authority> m_polkit;
};

#endif // KEEPASSXC_POLKIT_H
