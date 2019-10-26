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

    class PromptBase : public DBusObject
    {
        Q_OBJECT
    public:
        explicit PromptBase(Service* parent);

        virtual DBusReturn<void> prompt(const QString& windowId) = 0;

        virtual DBusReturn<void> dismiss();

    signals:
        void completed(bool dismissed, const QVariant& result);

    protected:
        QWindow* findWindow(const QString& windowId);
        Service* service() const;
    };

    class Collection;

    class DeleteCollectionPrompt : public PromptBase
    {
        Q_OBJECT

    public:
        explicit DeleteCollectionPrompt(Service* parent, Collection* coll);

        DBusReturn<void> prompt(const QString& windowId) override;

    private:
        QPointer<Collection> m_collection;
    };

    class CreateCollectionPrompt : public PromptBase
    {
        Q_OBJECT

    public:
        explicit CreateCollectionPrompt(Service* parent);

        DBusReturn<void> prompt(const QString& windowId) override;

    signals:
        void collectionCreated(Collection* coll);
    };

    class LockCollectionsPrompt : public PromptBase
    {
        Q_OBJECT
    public:
        explicit LockCollectionsPrompt(Service* parent, const QList<Collection*>& colls);

        DBusReturn<void> prompt(const QString& windowId) override;

    private:
        QList<QPointer<Collection>> m_collections;
    };

    class UnlockCollectionsPrompt : public PromptBase
    {
        Q_OBJECT
    public:
        explicit UnlockCollectionsPrompt(Service* parent, const QList<Collection*>& coll);

        DBusReturn<void> prompt(const QString& windowId) override;

    private:
        QList<QPointer<Collection>> m_collections;
    };

    class Item;
    class DeleteItemPrompt : public PromptBase
    {
        Q_OBJECT

    public:
        explicit DeleteItemPrompt(Service* parent, Item* item);

        DBusReturn<void> prompt(const QString& windowId) override;

    private:
        QPointer<Item> m_item;
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_PROMPT_H
