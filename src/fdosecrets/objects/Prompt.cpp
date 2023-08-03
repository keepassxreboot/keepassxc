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

#include "fdosecrets/objects/Collection.h"
#include "fdosecrets/objects/Item.h"
#include "fdosecrets/objects/Service.h"
#include "fdosecrets/objects/Session.h"
#include "fdosecrets/widgets/AccessControlDialog.h"

#include "FdoSecretsSettings.h"
#include "core/Entry.h"
#include "gui/MessageBox.h"

#include <QThread>
#include <QTimer>
#include <QWindow>

namespace FdoSecrets
{
    const PromptResult PromptResult::Pending{PromptResult::AsyncPending};

    PromptBase::PromptBase(Service* parent)
        : DBusObject(parent)
    {
        connect(this, &PromptBase::completed, this, &PromptBase::deleteLater);
    }

    QWindow* PromptBase::findWindow(const QString& windowId)
    {
        // find parent window, or nullptr if not found
        bool ok = false;
        WId wid = windowId.toULongLong(&ok, 0);
        QWindow* parent = nullptr;
        if (ok) {
            parent = QWindow::fromWinId(wid);
        }
        if (parent) {
            // parent is not the child of any object, so make sure it gets deleted at some point
            QObject::connect(this, &QObject::destroyed, parent, &QObject::deleteLater);
        }
        return parent;
    }

    Service* PromptBase::service() const
    {
        return qobject_cast<Service*>(parent());
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
            if (!res.isPending()) {
                finishPrompt(res.isDismiss());
            }
        });
        return {};
    }

    DBusResult PromptBase::dismiss()
    {
        finishPrompt(true);
        return {};
    }

    QVariant PromptBase::currentResult() const
    {
        return "";
    }

    void PromptBase::finishPrompt(bool dismissed)
    {
        if (m_signalSent) {
            return;
        }
        m_signalSent = true;
        emit completed(dismissed, currentResult());
    }

    DeleteCollectionPrompt::DeleteCollectionPrompt(Service* parent, Collection* coll)
        : PromptBase(parent)
        , m_collection(coll)
    {
    }

    PromptResult DeleteCollectionPrompt::promptSync(const DBusClientPtr&, const QString& windowId)
    {
        MessageBox::OverrideParent override(findWindow(windowId));

        // if m_collection is already gone then treat as deletion accepted
        auto accepted = true;
        if (m_collection) {
            accepted = m_collection->doDelete();
        }
        return PromptResult::accepted(accepted);
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

    PromptResult CreateCollectionPrompt::promptSync(const DBusClientPtr&, const QString& windowId)
    {
        MessageBox::OverrideParent override(findWindow(windowId));

        bool created = false;
        // collection with the alias may be created since the prompt was created
        auto ret = service()->readAlias(m_alias, m_coll);
        if (ret.err()) {
            return ret;
        }
        if (!m_coll) {
            created = true;
            m_coll = service()->doNewDatabase();
            if (!m_coll) {
                return PromptResult::accepted(false);
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
            ret = m_coll->addAlias(m_alias);
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

    PromptResult LockCollectionsPrompt::promptSync(const DBusClientPtr&, const QString& windowId)
    {
        MessageBox::OverrideParent override(findWindow(windowId));

        for (const auto& c : asConst(m_collections)) {
            if (c) {
                auto accepted = c->doLock();
                if (accepted) {
                    m_locked << c->objectPath();
                }
            }
        }
        return PromptResult::accepted(m_locked.size() == m_collections.size());
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

    PromptResult UnlockPrompt::promptSync(const DBusClientPtr& client, const QString& windowId)
    {
        MessageBox::OverrideParent override(findWindow(windowId));

        // for use in unlockItems
        m_windowId = windowId;
        m_client = client;

        // first unlock any collections
        bool waitingForCollections = false;
        for (const auto& c : asConst(m_collections)) {
            if (c) {
                connect(c, &Collection::doneUnlockCollection, this, &UnlockPrompt::collectionUnlockFinished);
                // doUnlock is nonblocking, execution will continue in collectionUnlockFinished
                // it is ok to call doUnlock multiple times before it's actually unlocked by the user
                c->doUnlock();
                waitingForCollections = true;
            }
        }

        // unlock items directly if no collection unlocking pending
        // o.w. doing it in collectionUnlockFinished
        if (!waitingForCollections) {
            unlockItems();
        }

        return PromptResult::Pending;
    }

    void UnlockPrompt::collectionUnlockFinished(bool accepted)
    {
        auto coll = qobject_cast<Collection*>(sender());
        if (!coll) {
            return;
        }

        // one shot
        coll->disconnect(this);

        if (!m_collections.contains(coll)) {
            // should not happen
            return;
        }

        if (accepted) {
            m_unlocked << coll->objectPath();
        } else {
            m_numRejected += 1;
            // no longer need to unlock the item if its containing collection didn't unlock.
            m_items.remove(coll);
        }

        // if we got response for all collections
        if (m_numRejected + m_unlocked.size() == m_collections.size()) {
            // next step is to unlock items
            unlockItems();
        }
    }

    void UnlockPrompt::unlockItems()
    {
        auto client = m_client.lock();
        if (!client) {
            // client already gone
            return;
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
            QString app = tr("%1 (PID: %2)").arg(client->name()).arg(client->pid());
            auto ac = new AccessControlDialog(
                findWindow(m_windowId), entries, app, client->processInfo(), AuthOption::Remember);
            connect(ac, &AccessControlDialog::finished, this, &UnlockPrompt::itemUnlockFinished);
            connect(ac, &AccessControlDialog::finished, ac, &AccessControlDialog::deleteLater);
            ac->open();
        } else {
            itemUnlockFinished({}, AuthDecision::Undecided);
        }
    }

    void UnlockPrompt::itemUnlockFinished(const QHash<Entry*, AuthDecision>& decisions, AuthDecision forFutureEntries)
    {
        auto client = m_client.lock();
        if (!client) {
            // client already gone
            qDebug() << "DBus client gone before item unlocking finish";
            return;
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
        // if anything is not unlocked, treat the whole prompt as dismissed
        // so the client has a chance to handle the error
        finishPrompt(m_numRejected > 0);
    }

    DeleteItemPrompt::DeleteItemPrompt(Service* parent, Item* item)
        : PromptBase(parent)
        , m_item(item)
    {
    }

    PromptResult DeleteItemPrompt::promptSync(const DBusClientPtr&, const QString& windowId)
    {
        MessageBox::OverrideParent override(findWindow(windowId));

        // if m_item is gone, assume it's already deleted
        bool deleted = true;
        if (m_item) {
            deleted = m_item->doDelete();
        }
        return PromptResult::accepted(deleted);
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
        , m_item(nullptr)
        // session aliveness also need to be tracked, for potential use later in updateItem
        , m_sess(m_secret.session)
    {
    }

    QVariant CreateItemPrompt::currentResult() const
    {
        return QVariant::fromValue(DBusMgr::objectPathSafe(m_item));
    }

    PromptResult CreateItemPrompt::promptSync(const DBusClientPtr& client, const QString& windowId)
    {
        if (!m_coll) {
            return PromptResult::accepted(false);
        }

        // save a weak reference to the client which may be used asynchronously later
        m_client = client;

        // give the user a chance to unlock the collection
        // UnlockPrompt will handle the case of collection already unlocked
        auto prompt = PromptBase::Create<UnlockPrompt>(service(), QSet<Collection*>{m_coll.data()}, QSet<Item*>{});
        if (!prompt) {
            return DBusResult{QDBusError::InternalError};
        }
        // postpone anything after the prompt
        connect(prompt, &PromptBase::completed, this, [this, windowId](bool dismissed) {
            if (dismissed) {
                finishPrompt(dismissed);
            } else {
                auto res = createItem(windowId);
                if (res.err()) {
                    qWarning() << "FdoSecrets:" << res;
                    finishPrompt(true);
                }
            }
        });

        auto ret = prompt->prompt(client, windowId);
        if (ret.err()) {
            return ret;
        }
        return PromptResult::Pending;
    }

    DBusResult CreateItemPrompt::createItem(const QString& windowId)
    {
        auto client = m_client.lock();
        if (!client) {
            // client already gone
            return {};
        }

        if (!m_coll) {
            return DBusResult{DBUS_ERROR_SECRET_NO_SUCH_OBJECT};
        }

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
        // postpone anything after the confirmation
        connect(prompt, &PromptBase::completed, this, [this](bool dismissed) {
            if (dismissed) {
                finishPrompt(dismissed);
            } else {
                auto res = updateItem();
                if (res.err()) {
                    qWarning() << "FdoSecrets:" << res;
                    finishPrompt(true);
                }
            }
        });

        auto ret = prompt->prompt(client, windowId);
        if (ret.err()) {
            return ret;
        }
        return {};
    }

    DBusResult CreateItemPrompt::updateItem()
    {
        auto client = m_client.lock();
        if (!client) {
            // client already gone
            return {};
        }

        if (!m_sess || m_sess != m_secret.session) {
            return DBusResult(DBUS_ERROR_SECRET_NO_SESSION);
        }
        if (!m_item) {
            return {};
        }
        auto ret = m_item->setProperties(m_properties);
        if (ret.err()) {
            return ret;
        }
        ret = m_item->setSecret(client, m_secret);
        if (ret.err()) {
            return ret;
        }

        // finally can finish the prompt without dismissing it
        finishPrompt(false);
        return {};
    }
} // namespace FdoSecrets
