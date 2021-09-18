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

#ifndef KEEPASSXC_FDOSECRETSSETTINGS_H
#define KEEPASSXC_FDOSECRETSSETTINGS_H

#include <QSharedPointer>
#include <QUuid>

class Database;

namespace FdoSecrets
{

    class FdoSecretsSettings
    {
    public:
        FdoSecretsSettings() = default;
        static FdoSecretsSettings* instance();

        bool isEnabled() const;
        void setEnabled(bool enabled);

        bool showNotification() const;
        void setShowNotification(bool show);

        bool confirmDeleteItem() const;
        void setConfirmDeleteItem(bool confirm);

        bool confirmAccessItem() const;
        void setConfirmAccessItem(bool confirmAccessItem);

        bool unlockBeforeSearch() const;
        void setUnlockBeforeSearch(bool unlockBeforeSearch);

        // Per db settings

        QUuid exposedGroup(const QSharedPointer<Database>& db) const;
        void setExposedGroup(const QSharedPointer<Database>& db, const QUuid& group);
        QUuid exposedGroup(Database* db) const;
        void setExposedGroup(Database* db, const QUuid& group);

    private:
        static FdoSecretsSettings* m_instance;
    };

    inline FdoSecretsSettings* settings()
    {
        return FdoSecretsSettings::instance();
    }

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETSSETTINGS_H
