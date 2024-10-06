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

#ifndef KEEPASSXC_FDOSECRETS_PROMPT_H
#define KEEPASSXC_FDOSECRETS_PROMPT_H

#include "core/Global.h"
#include "fdosecrets/dbus/DBusClient.h"
#include "fdosecrets/dbus/DBusObject.h"

class DatabaseWidget;
class Entry;

namespace FdoSecrets
{

    class Service;

    class PromptBase : public DBusObject
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", DBUS_INTERFACE_SECRET_PROMPT_LITERAL)
    public:
        /**
         * D-Bus Methods
         */

        Q_INVOKABLE DBusResult prompt(const DBusClientPtr& client, const QString& windowId);
        Q_INVOKABLE DBusResult dismiss();

        template <typename PROMPT, typename... ARGS> static PromptBase* Create(Service* parent, ARGS&&... args)
        {
            QScopedPointer<PROMPT> res{new PROMPT(parent, std::forward<ARGS>(args)...)};
            if (!res->dbus()->registerObject(res.data())) {
                // internal error;
                return nullptr;
            }
            return res.take();
        }

        virtual DBusResult promptSync(const DBusClientPtr& client, const QString& windowId) = 0;

    signals:
        void completed(bool dismissed, const QVariant& result);

    protected:
        explicit PromptBase(Service* parent);

        virtual QVariant currentResult() const;

        Service* service() const;

        // For overriding MessageBox parent window in GUI
        class OverrideParentWindow
        {
        public:
            explicit OverrideParentWindow(PromptBase* prompt, const QString& newParentWindowId);
            ~OverrideParentWindow();

        private:
            PromptBase const* m_prompt;
            const QString m_oldParentWindowId;
        };

    private:
        bool m_signalSent = false;
    };

    class Collection;

    class DeleteCollectionPrompt : public PromptBase
    {
        Q_OBJECT
        friend class PromptBase;

        explicit DeleteCollectionPrompt(Service* parent, Collection* coll);

        DBusResult promptSync(const DBusClientPtr& client, const QString& windowId) override;

        QPointer<Collection> m_collection;
    };

    class CreateCollectionPrompt : public PromptBase
    {
        Q_OBJECT
        friend class PromptBase;

        explicit CreateCollectionPrompt(Service* parent, QVariantMap properties, QString alias);

        DBusResult promptSync(const DBusClientPtr& client, const QString& windowId) override;
        QVariant currentResult() const override;

        QVariantMap m_properties;
        QString m_alias;
        Collection* m_coll;
    };

    class LockCollectionsPrompt : public PromptBase
    {
        Q_OBJECT
        friend class PromptBase;

        explicit LockCollectionsPrompt(Service* parent, const QList<Collection*>& colls);

        DBusResult promptSync(const DBusClientPtr& client, const QString& windowId) override;
        QVariant currentResult() const override;

        QList<QPointer<Collection>> m_collections;
        QList<QDBusObjectPath> m_locked;
    };

    class DBusClient;
    class UnlockPrompt : public PromptBase
    {
        Q_OBJECT
        friend class PromptBase;

        explicit UnlockPrompt(Service* parent, const QSet<Collection*>& colls, const QSet<Item*>& items);

        DBusResult promptSync(const DBusClientPtr& client, const QString& windowId) override;
        QVariant currentResult() const override;

        QList<QPointer<Collection>> m_collections;
        QHash<Collection*, QList<QPointer<Item>>> m_items;
        QHash<QUuid, Item*> m_entryToItems;

        QList<QDBusObjectPath> m_unlocked;
        int m_numRejected = 0;
    };

    class Item;
    class DeleteItemPrompt : public PromptBase
    {
        Q_OBJECT
        friend class PromptBase;

        explicit DeleteItemPrompt(Service* parent, Item* item);

        DBusResult promptSync(const DBusClientPtr& client, const QString& windowId) override;

        QPointer<Item> m_item;
    };

    class CreateItemPrompt : public PromptBase
    {
        Q_OBJECT
        friend class PromptBase;

        explicit CreateItemPrompt(Service* parent,
                                  Collection* coll,
                                  QVariantMap properties,
                                  Secret secret,
                                  bool replace);

        DBusResult promptSync(const DBusClientPtr& client, const QString& windowId) override;
        QVariant currentResult() const override;

        DBusResult createItem(const DBusClientPtr& client, const QString& windowId);

        QPointer<Collection> m_coll;
        QVariantMap m_properties;
        Secret m_secret;
        bool m_replace;

        QPointer<Item> m_item;
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_PROMPT_H
