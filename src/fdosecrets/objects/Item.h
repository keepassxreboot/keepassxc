/*
 *  Copyright (C) 2019 Aetf <aetf@unlimitedcodeworks.xyz>
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

#ifndef KEEPASSXC_FDOSECRETS_ITEM_H
#define KEEPASSXC_FDOSECRETS_ITEM_H

#include "fdosecrets/objects/DBusObject.h"
#include "fdosecrets/objects/adaptors/ItemAdaptor.h"

#include <QPointer>

class Entry;

namespace FdoSecrets
{

    namespace ItemAttributes
    {
        constexpr const auto UuidKey = "Uuid";
        constexpr const auto PathKey = "Path";
    } // namespace ItemAttributes

    class Session;
    class Collection;
    class PromptBase;

    class Item : public DBusObjectHelper<Item, ItemAdaptor>
    {
        Q_OBJECT

        explicit Item(Collection* parent, Entry* backend);

    public:
        /**
         * @brief Create a new instance of `Item`.
         * @param parent the owning `Collection`
         * @param backend the `Entry` containing the data
         * @return pointer to newly created Item, or nullptr if error
         * This may be caused by
         *   - DBus path registration error
         */
        static Item* Create(Collection* parent, Entry* backend);

        DBusReturn<bool> locked() const;

        DBusReturn<const StringStringMap> attributes() const;
        DBusReturn<void> setAttributes(const StringStringMap& attrs);

        DBusReturn<QString> label() const;
        DBusReturn<void> setLabel(const QString& label);

        DBusReturn<qulonglong> created() const;

        DBusReturn<qulonglong> modified() const;

        DBusReturn<PromptBase*> deleteItem();
        DBusReturn<SecretStruct> getSecret(Session* session);
        DBusReturn<void> setSecret(const SecretStruct& secret);

    signals:
        void itemChanged();
        void itemAboutToDelete();

    public:
        static const QSet<QString> ReadOnlyAttributes;

        DBusReturn<void> setProperties(const QVariantMap& properties);

        Entry* backend() const;
        Collection* collection() const;
        Service* service() const;

        /**
         * Compute the entry path relative to the exposed group
         * @return the entry path
         */
        QString path() const;

        /**
         * If the containing db does not have recycle bin enabled,
         * or the entry is already in the recycle bin (not possible for item, though),
         * the delete is permanent
         * @return true if delete is permanent
         */
        bool isDeletePermanent() const;

    public slots:
        void doDelete();

        /**
         * @brief Register self on DBus
         * @return
         */
        bool registerSelf();

        /**
         * Check if the backend is a valid object, send error reply if not.
         * @return No error if the backend is valid.
         */
        DBusReturn<void> ensureBackend() const;

        /**
         * Ensure the database is unlocked, send error reply if locked.
         * @return true if the database is locked
         */
        DBusReturn<void> ensureUnlocked() const;

    private:
        QPointer<Entry> m_backend;
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_ITEM_H
