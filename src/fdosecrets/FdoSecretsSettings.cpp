/*
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

namespace Keys
{
    namespace Db
    {
        constexpr auto FdoSecretsExposedGroup = "FDO_SECRETS_EXPOSED_GROUP";
    } // namespace Db

} // namespace Keys

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

    QUuid FdoSecretsSettings::exposedGroup(const QSharedPointer<Database>& db) const
    {
        return exposedGroup(db.data());
    }

    void FdoSecretsSettings::setExposedGroup(const QSharedPointer<Database>& db,
                                             const QUuid& group) // clazy:exclude=function-args-by-value
    {
        setExposedGroup(db.data(), group);
    }

    QUuid FdoSecretsSettings::exposedGroup(Database* db) const
    {
        return {db->metadata()->customData()->value(Keys::Db::FdoSecretsExposedGroup)};
    }

    void FdoSecretsSettings::setExposedGroup(Database* db, const QUuid& group) // clazy:exclude=function-args-by-value
    {
        db->metadata()->customData()->set(Keys::Db::FdoSecretsExposedGroup, group.toString());
    }

} // namespace FdoSecrets
