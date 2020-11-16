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

#include "fdosecrets/objects/DBusObject.h"
#include "fdosecrets/objects/adaptors/PromptAdaptor.h"

#include <QPointer>

class QWindow;

class DatabaseWidget;

namespace FdoSecrets
{

    class Service;

    class PromptBase : public DBusObjectHelper<PromptBase, PromptAdaptor>
    {
        Q_OBJECT
    public:
        virtual DBusReturn<void> prompt(const QString& windowId) = 0;

        virtual DBusReturn<void> dismiss();

    signals:
        void completed(bool dismissed, const QVariant& result);

    protected:
        explicit PromptBase(Service* parent);

        bool registerSelf();
        QWindow* findWindow(const QString& windowId);
        Service* service() const;
    };

    class Collection;

    class DeleteCollectionPrompt : public PromptBase
    {
        Q_OBJECT

        explicit DeleteCollectionPrompt(Service* parent, Collection* coll);

    public:
        static DBusReturn<DeleteCollectionPrompt*> Create(Service* parent, Collection* coll);

        DBusReturn<void> prompt(const QString& windowId) override;

    private:
        QPointer<Collection> m_collection;
    };

    class CreateCollectionPrompt : public PromptBase
    {
        Q_OBJECT

        explicit CreateCollectionPrompt(Service* parent);

    public:
        static DBusReturn<CreateCollectionPrompt*> Create(Service* parent);

        DBusReturn<void> prompt(const QString& windowId) override;
        DBusReturn<void> dismiss() override;

    signals:
        void collectionCreated(Collection* coll);
    };

    class LockCollectionsPrompt : public PromptBase
    {
        Q_OBJECT

        explicit LockCollectionsPrompt(Service* parent, const QList<Collection*>& colls);

    public:
        static DBusReturn<LockCollectionsPrompt*> Create(Service* parent, const QList<Collection*>& colls);

        DBusReturn<void> prompt(const QString& windowId) override;
        DBusReturn<void> dismiss() override;

    private:
        QList<QPointer<Collection>> m_collections;
        QList<QDBusObjectPath> m_locked;
    };

    class UnlockCollectionsPrompt : public PromptBase
    {
        Q_OBJECT

        explicit UnlockCollectionsPrompt(Service* parent, const QList<Collection*>& coll);

    public:
        static DBusReturn<UnlockCollectionsPrompt*> Create(Service* parent, const QList<Collection*>& coll);

        DBusReturn<void> prompt(const QString& windowId) override;
        DBusReturn<void> dismiss() override;

    private slots:
        void collectionUnlockFinished(bool accepted);

    private:
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
        static DBusReturn<DeleteItemPrompt*> Create(Service* parent, Item* item);

        DBusReturn<void> prompt(const QString& windowId) override;

    private:
        QPointer<Item> m_item;
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_PROMPT_H
