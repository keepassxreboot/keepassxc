/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2018 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include "FdoSecretsSettings.h"

#include "core/Config.h"
#include "core/Database.h"
#include "core/Metadata.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>

namespace FdoSecrets
{

    FdoSecretsSettings* FdoSecretsSettings::m_instance = nullptr;

    FdoSecretsSettings* FdoSecretsSettings::instance()
    {
        if (!m_instance) {
            m_instance = new FdoSecretsSettings;
        }
        return m_instance;
    }

    bool FdoSecretsSettings::isEnabled() const
    {
        return config()->get(Config::FdoSecrets_Enabled).toBool();
    }

    void FdoSecretsSettings::setEnabled(bool enabled)
    {
        config()->set(Config::FdoSecrets_Enabled, enabled);
    }

    bool FdoSecretsSettings::showNotification() const
    {
        return config()->get(Config::FdoSecrets_ShowNotification).toBool();
    }

    void FdoSecretsSettings::setShowNotification(bool show)
    {
        config()->set(Config::FdoSecrets_ShowNotification, show);
    }

    bool FdoSecretsSettings::confirmDeleteItem() const
    {
        return config()->get(Config::FdoSecrets_ConfirmDeleteItem).toBool();
    }

    void FdoSecretsSettings::setConfirmDeleteItem(bool confirm)
    {
        config()->set(Config::FdoSecrets_ConfirmDeleteItem, confirm);
    }

    bool FdoSecretsSettings::confirmAccessItem() const
    {
        return config()->get(Config::FdoSecrets_ConfirmAccessItem).toBool();
    }

    void FdoSecretsSettings::setConfirmAccessItem(bool confirmAccessItem)
    {
        config()->set(Config::FdoSecrets_ConfirmAccessItem, confirmAccessItem);
    }

    bool FdoSecretsSettings::unlockBeforeSearch() const
    {
        return config()->get(Config::FdoSecrets_UnlockBeforeSearch).toBool();
    }

    void FdoSecretsSettings::setUnlockBeforeSearch(bool unlockBeforeSearch)
    {
        config()->set(Config::FdoSecrets_UnlockBeforeSearch, unlockBeforeSearch);
    }

    QStringList FdoSecretsSettings::authorizedClients() const
    {
        return config()->get(Config::FdoSecrets_AuthorizedClients).toStringList();
    }

    void FdoSecretsSettings::setAuthorizedClients(const QStringList& authorizedClients)
    {
        config()->set(Config::FdoSecrets_AuthorizedClients, authorizedClients);
    }

    bool FdoSecretsSettings::isClientAuthorized(const QString& exePath) const
    {
        const auto list = authorizedClients();
        if (list.isEmpty() || exePath.isEmpty()) {
            return false;
        }

        const QString canonicalExe = QFileInfo(exePath).canonicalFilePath();

        for (const auto& item : list) {
            const int separatorIdx = item.lastIndexOf(':');
            if (separatorIdx <= 0) {
                continue;
            }

            const QString allowedPath = item.left(separatorIdx).trimmed();
            const QString expectedHash = item.mid(separatorIdx + 1).trimmed();

            if (allowedPath == exePath || (!canonicalExe.isEmpty() && allowedPath == canonicalExe)) {
                QFile file(canonicalExe.isEmpty() ? exePath : canonicalExe);
                if (!file.open(QIODevice::ReadOnly)) {
                    continue;
                }

                QCryptographicHash hash(QCryptographicHash::Sha256);
                if (!hash.addData(&file)) {
                    continue;
                }

                const QString computedHash = QString::fromLatin1(hash.result().toHex());
                if (expectedHash.compare(computedHash, Qt::CaseInsensitive) == 0) {
                    return true;
                }
            }
        }
        return false;
    }

    QUuid FdoSecretsSettings::exposedGroup(const QSharedPointer<Database>& db) const
    {
        return exposedGroup(db.data());
    }

    void FdoSecretsSettings::setExposedGroup(const QSharedPointer<Database>& db, const QUuid& group)
    {
        setExposedGroup(db.data(), group);
    }

    QUuid FdoSecretsSettings::exposedGroup(Database* db) const
    {
        return QUuid(db->metadata()->customData()->value(CustomData::FdoSecretsExposedGroup));
    }

    void FdoSecretsSettings::setExposedGroup(Database* db, const QUuid& group)
    {
        db->metadata()->customData()->set(CustomData::FdoSecretsExposedGroup, group.toString());
    }

} // namespace FdoSecrets
