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

#include "fdosecrets/dbus/DBusObject.h"

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

    class Item : public DBusObject
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", DBUS_INTERFACE_SECRET_ITEM)

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

        Q_INVOKABLE DBusResult locked(bool& locked) const;

        Q_INVOKABLE DBusResult attributes(StringStringMap& attrs) const;
        Q_INVOKABLE DBusResult setAttributes(const StringStringMap& attrs);

        Q_INVOKABLE DBusResult label(QString& label) const;
        Q_INVOKABLE DBusResult setLabel(const QString& label);

        Q_INVOKABLE DBusResult created(qulonglong& created) const;

        Q_INVOKABLE DBusResult modified(qulonglong& modified) const;

        Q_INVOKABLE DBusResult deleteItem(PromptBase*& prompt);
        Q_INVOKABLE DBusResult getSecret(Session* session, Secret& secret);
        Q_INVOKABLE DBusResult setSecret(const Secret& secret);

    signals:
        void itemChanged();
        void itemAboutToDelete();

    public:
        static const QSet<QString> ReadOnlyAttributes;

        DBusResult setProperties(const QVariantMap& properties);

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
         * Check if the backend is a valid object, send error reply if not.
         * @return No error if the backend is valid.
         */
        DBusResult ensureBackend() const;

        /**
         * Ensure the database is unlocked, send error reply if locked.
         * @return true if the database is locked
         */
        DBusResult ensureUnlocked() const;

    private:
        QPointer<Entry> m_backend;
    };

} // namespace FdoSecrets
Q_DECLARE_METATYPE(FdoSecrets::ItemSecretMap);

#endif // KEEPASSXC_FDOSECRETS_ITEM_H
