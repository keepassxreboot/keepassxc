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

#include "fdosecrets/dbus/DBusObject.h"

#include <QPointer>

class QWindow;

class DatabaseWidget;

namespace FdoSecrets
{

    class Service;

    class PromptBase : public DBusObject
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", DBUS_INTERFACE_SECRET_PROMPT)
    public:
        Q_INVOKABLE virtual DBusResult prompt(const QString& windowId) = 0;

        Q_INVOKABLE virtual DBusResult dismiss();

        template<typename PROMPT, typename ...ARGS>
        static PromptBase* Create(Service* parent, ARGS&&... args)
        {
            QScopedPointer<PROMPT> res{new PROMPT(parent, std::forward<ARGS>(args)...)};
            if (res->dbus().registerObject(res.data())) {
                // internal error;
                return nullptr;
            }
            return res.take();
        }

    signals:
        void completed(bool dismissed, const QVariant& result);

    protected:
        explicit PromptBase(Service* parent);

        QWindow* findWindow(const QString& windowId);
        Service* service() const;
    };

    class Collection;

    class DeleteCollectionPrompt : public PromptBase
    {
        Q_OBJECT

        explicit DeleteCollectionPrompt(Service* parent, Collection* coll);

    public:
        DBusResult prompt(const QString& windowId) override;

    private:
        friend class PromptBase;

        QPointer<Collection> m_collection;
    };

    class CreateCollectionPrompt : public PromptBase
    {
        Q_OBJECT

        explicit CreateCollectionPrompt(Service* parent, QVariantMap properties, QString alias);

    public:
        DBusResult prompt(const QString& windowId) override;
        DBusResult dismiss() override;

    private:
        friend class PromptBase;

        QVariantMap m_properties;
        QString m_alias;
    };

    class LockCollectionsPrompt : public PromptBase
    {
        Q_OBJECT

        explicit LockCollectionsPrompt(Service* parent, const QList<Collection*>& colls);

    public:
        DBusResult prompt(const QString& windowId) override;
        DBusResult dismiss() override;

    private:
        friend class PromptBase;

        QList<QPointer<Collection>> m_collections;
        QList<QDBusObjectPath> m_locked;
    };

    class UnlockCollectionsPrompt : public PromptBase
    {
        Q_OBJECT

        explicit UnlockCollectionsPrompt(Service* parent, const QList<Collection*>& coll);

    public:
        DBusResult prompt(const QString& windowId) override;
        DBusResult dismiss() override;

    private slots:
        void collectionUnlockFinished(bool accepted);

    private:
        friend class PromptBase;

        QList<QPointer<Collection>> m_collections;
        QList<QDBusObjectPath> m_unlocked;
        int m_numRejected = 0;
    };

    class Item;
    class DeleteItemPrompt : public PromptBase
    {
        Q_OBJECT

        explicit DeleteItemPrompt(Service* parent, Item* item);

    public:
        DBusResult prompt(const QString& windowId) override;

    private:
        friend class PromptBase;

        QPointer<Item> m_item;
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_PROMPT_H
