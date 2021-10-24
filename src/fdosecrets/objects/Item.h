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

#include "fdosecrets/dbus/DBusClient.h"
#include "fdosecrets/dbus/DBusObject.h"

class Entry;

namespace FdoSecrets
{

    namespace ItemAttributes
    {
        constexpr const auto UuidKey = "Uuid";
        constexpr const auto PathKey = "Path";
        constexpr const auto TotpKey = "TOTP";
    } // namespace ItemAttributes

    class Session;
    class Collection;
    class PromptBase;

    class Item : public DBusObject
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", DBUS_INTERFACE_SECRET_ITEM_LITERAL)

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

        Q_INVOKABLE DBUS_PROPERTY DBusResult locked(const DBusClientPtr& client, bool& locked) const;

        Q_INVOKABLE DBUS_PROPERTY DBusResult attributes(StringStringMap& attrs) const;
        Q_INVOKABLE DBusResult setAttributes(const StringStringMap& attrs);

        Q_INVOKABLE DBUS_PROPERTY DBusResult label(QString& label) const;
        Q_INVOKABLE DBusResult setLabel(const QString& label);

        Q_INVOKABLE DBUS_PROPERTY DBusResult created(qulonglong& created) const;

        Q_INVOKABLE DBUS_PROPERTY DBusResult modified(qulonglong& modified) const;

        Q_INVOKABLE DBusResult remove(PromptBase*& prompt);
        Q_INVOKABLE DBusResult getSecret(const DBusClientPtr& client, Session* session, Secret& secret);
        Q_INVOKABLE DBusResult setSecret(const DBusClientPtr& client, const Secret& secret);

    signals:
        void itemChanged();
        void itemAboutToDelete();

    public:
        static const QSet<QString> ReadOnlyAttributes;

        DBusResult getSecretNoNotification(const DBusClientPtr& client, Session* session, Secret& secret) const;
        DBusResult setProperties(const QVariantMap& properties);

        Entry* backend() const;
        Collection* collection() const;
        Service* service() const;

        /**
         * Compute the entry path relative to the exposed group
         * @return the entry path
         */
        QString path() const;

    public slots:
        // will actually delete the entry in KPXC
        bool doDelete();

        // Only delete from dbus, will remove self. Do not affect database in KPXC
        void removeFromDBus();

    private slots:
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
