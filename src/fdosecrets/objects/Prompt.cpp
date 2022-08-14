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

#include "Prompt.h"

#include "fdosecrets/FdoSecretsPlugin.h"
#include "fdosecrets/objects/Collection.h"
#include "fdosecrets/objects/Item.h"
#include "fdosecrets/objects/Service.h"
#include "fdosecrets/objects/Session.h"

#include "FdoSecretsSettings.h"
#include "core/Entry.h"

#include <QThread>
#include <QTimer>

namespace FdoSecrets
{
    PromptBase::PromptBase(Service* parent)
        : DBusObject(parent)
    {
        connect(this, &PromptBase::completed, this, &PromptBase::deleteLater);
    }

    Service* PromptBase::service() const
    {
        return qobject_cast<Service*>(parent());
    }

    PromptBase::OverrideParentWindow::OverrideParentWindow(PromptBase* prompt, const QString& newParentWindowId)
        : m_prompt(prompt)
        , m_oldParentWindowId(prompt->service()->plugin()->overrideMessageBoxParent(newParentWindowId))
    {
    }

    PromptBase::OverrideParentWindow::~OverrideParentWindow()
    {
        m_prompt->service()->plugin()->overrideMessageBoxParent(m_oldParentWindowId);
    }

    DBusResult PromptBase::prompt(const DBusClientPtr& client, const QString& windowId)
    {
        if (thread() != QThread::currentThread()) {
            DBusResult ret;
            QMetaObject::invokeMethod(
                this, "prompt", Qt::BlockingQueuedConnection, Q_ARG(QString, windowId), Q_RETURN_ARG(DBusResult, ret));
            return ret;
        }

        QWeakPointer<DBusClient> weak = client;
        // execute the actual prompt method in event loop to avoid block this method
        QTimer::singleShot(0, this, [this, weak, windowId]() {
            auto c = weak.lock();
            if (!c) {
                return;
            }

            if (m_signalSent) {
                return;
            }

            auto res = promptSync(c, windowId);
            if (m_signalSent) {
                return;
            }
            m_signalSent = true;
            emit completed(!res.ok(), currentResult());
        });
        return {};
    }

    DBusResult PromptBase::dismiss()
    {
        m_signalSent = true;
        emit completed(true, currentResult());
        return {};
    }

    QVariant PromptBase::currentResult() const
    {
        return "";
    }

    DeleteCollectionPrompt::DeleteCollectionPrompt(Service* parent, Collection* coll)
        : PromptBase(parent)
        , m_collection(coll)
    {
    }

    DBusResult DeleteCollectionPrompt::promptSync(const DBusClientPtr&, const QString& windowId)
    {
        OverrideParentWindow override(this, windowId);

        if (m_collection) {
            // Collection removal is just disconnecting it from D-Bus.
            // GUI then reacts with potentially closing the database.
            m_collection->removeFromDBus();
        }
        return {};
    }

    CreateCollectionPrompt::CreateCollectionPrompt(Service* parent, QVariantMap properties, QString alias)
        : PromptBase(parent)
        , m_properties(std::move(properties))
        , m_alias(std::move(alias))
    {
    }

    QVariant CreateCollectionPrompt::currentResult() const
    {
        return QVariant::fromValue(DBusMgr::objectPathSafe(m_coll));
    }

    DBusResult CreateCollectionPrompt::promptSync(const DBusClientPtr& client, const QString& windowId)
    {
        OverrideParentWindow override(this, windowId);

        bool created = false;
        // collection with the alias may be created since the prompt was created
        auto ret = service()->readAlias(m_alias, m_coll);
        if (ret.err()) {
            return ret;
        }

        if (!m_coll) {
            created = true;
            m_coll = service()->doNewDatabase(client);
            if (!m_coll) {
                return {QDBusError::AccessDenied};
            }
        }

        ret = m_coll->setProperties(m_properties);
        if (ret.err()) {
            if (created) {
                m_coll->removeFromDBus();
            }
            return ret;
        }
        if (!m_alias.isEmpty()) {
            ret = service()->setAlias(m_alias, m_coll);
            if (ret.err()) {
                if (created) {
                    m_coll->removeFromDBus();
                }
                return ret;
            }
        }

        return {};
    }

    LockCollectionsPrompt::LockCollectionsPrompt(Service* parent, const QList<Collection*>& colls)
        : PromptBase(parent)
    {
        m_collections.reserve(colls.size());
        for (const auto& c : asConst(colls)) {
            m_collections << c;
        }
    }

    QVariant LockCollectionsPrompt::currentResult() const
    {
        return QVariant::fromValue(m_locked);
    }

    DBusResult LockCollectionsPrompt::promptSync(const DBusClientPtr& client, const QString& windowId)
    {
        OverrideParentWindow override(this, windowId);

        for (const auto& c : asConst(m_collections)) {
            if (c) {
                auto accepted = c->doLock(client);
                if (accepted) {
                    m_locked << c->objectPath();
                }
            }
        }
        return {m_locked.size() == m_collections.size() ? QDBusError::NoError : QDBusError::AccessDenied};
    }

    UnlockPrompt::UnlockPrompt(Service* parent, const QSet<Collection*>& colls, const QSet<Item*>& items)
        : PromptBase(parent)
    {
        m_collections.reserve(colls.size());
        for (const auto& coll : asConst(colls)) {
            m_collections << coll;
        }
        for (const auto& item : asConst(items)) {
            m_items[item->collection()] << item;
        }
    }

    QVariant UnlockPrompt::currentResult() const
    {
        return QVariant::fromValue(m_unlocked);
    }

    DBusResult UnlockPrompt::promptSync(const DBusClientPtr& client, const QString& windowId)
    {
        OverrideParentWindow override(this, windowId);

        // first unlock any collections
        for (const auto& c : asConst(m_collections)) {
            if (c) {
                bool accepted = c->doUnlock(client);
                if (accepted) {
                    m_unlocked << c->objectPath();
                } else {
                    m_numRejected += 1;
                    // no longer need to unlock the item if its containing collection didn't unlock.
                    m_items.remove(c);
                }
            }
        }

        // flatten to list of entries
        QList<Entry*> entries;
        for (const auto& itemsPerColl : asConst(m_items)) {
            for (const auto& item : itemsPerColl) {
                if (!item) {
                    m_numRejected += 1;
                    continue;
                }
                auto entry = item->backend();
                auto uuid = entry->uuid();
                if (client->itemKnown(uuid) || !FdoSecrets::settings()->confirmAccessItem()) {
                    if (!client->itemAuthorized(uuid)) {
                        m_numRejected += 1;
                    }
                    // Already saw this entry
                    continue;
                }
                m_entryToItems[uuid] = item.data();
                entries << entry;
            }
        }

        if (!entries.isEmpty()) {
            QHash<Entry*, AuthDecision> decisions{};
            AuthDecision forFutureEntries{};
            if (!service()->plugin()->requestEntriesUnlock(client, windowId, entries, decisions, forFutureEntries)) {
                return {QDBusError::InternalError};
            }

            for (auto it = decisions.constBegin(); it != decisions.constEnd(); ++it) {
                auto entry = it.key();
                auto uuid = entry->uuid();
                // get back the corresponding item
                auto item = m_entryToItems.value(uuid);
                if (!item) {
                    continue;
                }

                // set auth
                client->setItemAuthorized(uuid, it.value());

                if (client->itemAuthorized(uuid)) {
                    m_unlocked += item->objectPath();
                } else {
                    m_numRejected += 1;
                }
            }
            if (forFutureEntries != AuthDecision::Undecided) {
                client->setAllAuthorized(forFutureEntries);
            }
        }

        // if anything is not unlocked, treat the whole prompt as dismissed
        // so the client has a chance to handle the error
        return {!m_numRejected ? QDBusError::NoError : QDBusError::AccessDenied};
    }

    DeleteItemPrompt::DeleteItemPrompt(Service* parent, Item* item)
        : PromptBase(parent)
        , m_item(item)
    {
    }

    DBusResult DeleteItemPrompt::promptSync(const DBusClientPtr& client, const QString& windowId)
    {
        OverrideParentWindow override(this, windowId);

        // if m_item is gone, assume it's already deleted
        bool deleted = true;
        if (m_item) {
            deleted = m_item->doDelete(client);
        }
        return {deleted ? QDBusError::NoError : QDBusError::AccessDenied};
    }

    CreateItemPrompt::CreateItemPrompt(Service* parent,
                                       Collection* coll,
                                       QVariantMap properties,
                                       Secret secret,
                                       bool replace)
        : PromptBase(parent)
        , m_coll(coll)
        , m_properties(std::move(properties))
        , m_secret(std::move(secret))
        , m_replace(replace)
        , m_item{nullptr}
    {
    }

    QVariant CreateItemPrompt::currentResult() const
    {
        return QVariant::fromValue(DBusMgr::objectPathSafe(m_item));
    }

    DBusResult CreateItemPrompt::promptSync(const DBusClientPtr& client, const QString& windowId)
    {
        if (!m_coll) {
            return DBusResult{DBUS_ERROR_SECRET_NO_SUCH_OBJECT};
        }

        // give the user a chance to unlock the collection
        // UnlockPrompt will handle the case of collection already unlocked
        auto prompt = PromptBase::Create<UnlockPrompt>(service(), QSet<Collection*>{m_coll.data()}, QSet<Item*>{});
        if (!prompt) {
            return {QDBusError::InternalError};
        }

        auto ret = prompt->promptSync(client, windowId);
        if (ret.err()) {
            return ret;
        }

        auto res = createItem(client, windowId);
        if (res.err()) {
            qWarning() << "FdoSecrets:" << res;
        }

        return {res.ok() ? QDBusError::NoError : QDBusError::InternalError};
    }

    DBusResult CreateItemPrompt::createItem(const DBusClientPtr& client, const QString& windowId)
    {
        // get itemPath to create item and
        // try to find an existing item using attributes
        QString itemPath{};
        auto iterAttr = m_properties.find(DBUS_INTERFACE_SECRET_ITEM + ".Attributes");
        if (iterAttr != m_properties.end()) {
            // the actual value in iterAttr.value() is QDBusArgument, which represents a structure
            // and qt has no idea what this corresponds to.
            // we thus force a conversion to StringStringMap here. The conversion is registered in
            // DBusTypes.cpp
            auto attributes = iterAttr.value().value<StringStringMap>();

            itemPath = attributes.value(ItemAttributes::PathKey);

            // check existing item using attributes
            QList<Item*> existing;
            auto ret = m_coll->searchItems(client, attributes, existing);
            if (ret.err()) {
                return ret;
            }
            if (!existing.isEmpty() && m_replace) {
                m_item = existing.front();
            }
        }

        if (!m_item) {
            // the item doesn't exist yet, create it
            m_item = m_coll->doNewItem(client, itemPath);
            if (!m_item) {
                // may happen if entry somehow ends up in recycle bin
                return DBusResult{DBUS_ERROR_SECRET_NO_SUCH_OBJECT};
            }
        }

        // the item may be locked due to authorization
        // give the user a chance to unlock the item
        auto prompt = PromptBase::Create<UnlockPrompt>(service(), QSet<Collection*>{}, QSet<Item*>{m_item});
        if (!prompt) {
            return DBusResult{QDBusError::InternalError};
        }

        auto ret = prompt->promptSync(client, windowId);
        if (ret.err()) {
            return ret;
        }

        ret = m_item->setProperties(m_properties);
        if (ret.err()) {
            return ret;
        }

        ret = m_item->setSecret(client, m_secret);
        return ret;
    }

} // namespace FdoSecrets
