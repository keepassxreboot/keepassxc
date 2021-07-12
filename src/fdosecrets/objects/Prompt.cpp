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

#include "core/Entry.h"
#include "gui/MessageBox.h"

#include <QThread>
#include <QTimer>
#include <QWindow>

namespace FdoSecrets
{

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

    DBusResult PromptBase::dismiss()
    {
        emit completed(true, "");
        return {};
    }

    DeleteCollectionPrompt::DeleteCollectionPrompt(Service* parent, Collection* coll)
        : PromptBase(parent)
        , m_collection(coll)
    {
    }

    DBusResult DeleteCollectionPrompt::prompt(const DBusClientPtr&, const QString& windowId)
    {
        if (thread() != QThread::currentThread()) {
            DBusResult ret;
            QMetaObject::invokeMethod(
                this, "prompt", Qt::BlockingQueuedConnection, Q_ARG(QString, windowId), Q_RETURN_ARG(DBusResult, ret));
            return ret;
        }

        if (!m_collection) {
            return DBusResult(DBUS_ERROR_SECRET_NO_SUCH_OBJECT);
        }

        MessageBox::OverrideParent override(findWindow(windowId));
        // only need to delete in backend, collection will react itself.
        auto accepted = service()->doCloseDatabase(m_collection->backend());

        emit completed(!accepted, "");

        return {};
    }

    CreateCollectionPrompt::CreateCollectionPrompt(Service* parent, QVariantMap properties, QString alias)
        : PromptBase(parent)
        , m_properties(std::move(properties))
        , m_alias(std::move(alias))
    {
    }

    DBusResult CreateCollectionPrompt::prompt(const DBusClientPtr&, const QString& windowId)
    {
        if (thread() != QThread::currentThread()) {
            DBusResult ret;
            QMetaObject::invokeMethod(
                this, "prompt", Qt::BlockingQueuedConnection, Q_ARG(QString, windowId), Q_RETURN_ARG(DBusResult, ret));
            return ret;
        }

        MessageBox::OverrideParent override(findWindow(windowId));

        auto coll = service()->doNewDatabase();
        if (!coll) {
            return dismiss();
        }

        auto ret = coll->setProperties(m_properties);
        if (ret.err()) {
            coll->doDelete();
            return dismiss();
        }
        if (!m_alias.isEmpty()) {
            ret = coll->addAlias(m_alias);
            if (ret.err()) {
                coll->doDelete();
                return dismiss();
            }
        }

        emit completed(false, QVariant::fromValue(coll->objectPath()));

        return {};
    }

    DBusResult CreateCollectionPrompt::dismiss()
    {
        emit completed(true, QVariant::fromValue(DBusMgr::objectPathSafe(nullptr)));
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

    DBusResult LockCollectionsPrompt::prompt(const DBusClientPtr&, const QString& windowId)
    {
        if (thread() != QThread::currentThread()) {
            DBusResult ret;
            QMetaObject::invokeMethod(
                this, "prompt", Qt::BlockingQueuedConnection, Q_ARG(QString, windowId), Q_RETURN_ARG(DBusResult, ret));
            return ret;
        }

        MessageBox::OverrideParent override(findWindow(windowId));

        for (const auto& c : asConst(m_collections)) {
            if (c) {
                if (!c->doLock()) {
                    return dismiss();
                }
                m_locked << c->objectPath();
            }
        }

        emit completed(false, QVariant::fromValue(m_locked));

        return {};
    }

    DBusResult LockCollectionsPrompt::dismiss()
    {
        emit completed(true, QVariant::fromValue(m_locked));
        return {};
    }

    UnlockPrompt::UnlockPrompt(Service* parent, const QSet<Collection*>& colls, const QSet<Item*>& items)
        : PromptBase(parent)
    {
        m_collections.reserve(colls.size());
        for (const auto& coll : asConst(colls)) {
            m_collections << coll;
            connect(coll, &Collection::doneUnlockCollection, this, &UnlockPrompt::collectionUnlockFinished);
        }
        for (const auto& item : asConst(items)) {
            m_items[item->collection()] << item;
        }
    }

    DBusResult UnlockPrompt::prompt(const DBusClientPtr& client, const QString& windowId)
    {
        if (thread() != QThread::currentThread()) {
            DBusResult ret;
            QMetaObject::invokeMethod(
                this, "prompt", Qt::BlockingQueuedConnection, Q_ARG(QString, windowId), Q_RETURN_ARG(DBusResult, ret));
            return ret;
        }

        MessageBox::OverrideParent override(findWindow(windowId));

        // for use in unlockItems
        m_windowId = windowId;
        m_client = client;

        // first unlock any collections
        bool waitingForCollections = false;
        for (const auto& c : asConst(m_collections)) {
            if (c) {
                // doUnlock is nonblocking, execution will continue in collectionUnlockFinished
                c->doUnlock();
                waitingForCollections = true;
            }
        }

        // unlock items directly if no collection unlocking pending
        // o.w. do it in collectionUnlockFinished
        if (!waitingForCollections) {
            // do not block the current method
            QTimer::singleShot(0, this, &UnlockPrompt::unlockItems);
        }

        return {};
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
            // no longer need to unlock the item if its containing collection
            // didn't unlock.
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
        for (const auto& itemsPerColl : m_items.values()) {
            for (const auto& item : itemsPerColl) {
                if (!item) {
                    m_numRejected += 1;
                    continue;
                }
                auto entry = item->backend();
                if (client->itemKnown(entry->uuid())) {
                    if (!client->itemAuthorized(entry->uuid())) {
                        m_numRejected += 1;
                    }
                    continue;
                }
                // attach a temporary property so later we can get the item
                // back from the dialog's result
                entry->setProperty(FdoSecretsBackend, QVariant::fromValue(item.data()));
                entries << entry;
            }
        }
        if (!entries.isEmpty()) {
            QString app = tr("%1 (PID: %2)").arg(client->name()).arg(client->pid());
            auto ac = new AccessControlDialog(findWindow(m_windowId), entries, app, AuthOption::Remember);
            connect(ac, &AccessControlDialog::finished, this, &UnlockPrompt::itemUnlockFinished);
            connect(ac, &AccessControlDialog::finished, ac, &AccessControlDialog::deleteLater);
            ac->open();
        } else {
            itemUnlockFinished({});
        }
    }

    void UnlockPrompt::itemUnlockFinished(const QHash<Entry*, AuthDecision>& decisions)
    {
        auto client = m_client.lock();
        if (!client) {
            // client already gone
            return;
        }
        for (auto it = decisions.constBegin(); it != decisions.constEnd(); ++it) {
            auto entry = it.key();
            // get back the corresponding item
            auto item = entry->property(FdoSecretsBackend).value<Item*>();
            entry->setProperty(FdoSecretsBackend, {});
            Q_ASSERT(item);

            // set auth
            client->setItemAuthorized(entry->uuid(), it.value());

            if (client->itemAuthorized(entry->uuid())) {
                m_unlocked += item->objectPath();
            } else {
                m_numRejected += 1;
            }
        }
        // if anything is not unlocked, treat the whole prompt as dismissed
        // so the client has a chance to handle the error
        emit completed(m_numRejected > 0, QVariant::fromValue(m_unlocked));
    }

    DBusResult UnlockPrompt::dismiss()
    {
        emit completed(true, QVariant::fromValue(m_unlocked));
        return {};
    }

    DeleteItemPrompt::DeleteItemPrompt(Service* parent, Item* item)
        : PromptBase(parent)
        , m_item(item)
    {
    }

    DBusResult DeleteItemPrompt::prompt(const DBusClientPtr&, const QString& windowId)
    {
        if (thread() != QThread::currentThread()) {
            DBusResult ret;
            QMetaObject::invokeMethod(
                this, "prompt", Qt::BlockingQueuedConnection, Q_ARG(QString, windowId), Q_RETURN_ARG(DBusResult, ret));
            return ret;
        }

        MessageBox::OverrideParent override(findWindow(windowId));

        // delete item's backend. Item will be notified after the backend is deleted.
        if (m_item) {
            m_item->collection()->doDeleteEntries({m_item->backend()});
        }

        emit completed(false, "");

        return {};
    }

    CreateItemPrompt::CreateItemPrompt(Service* parent,
                                       Collection* coll,
                                       QVariantMap properties,
                                       Secret secret,
                                       QString itemPath,
                                       Item* existing)
        : PromptBase(parent)
        , m_coll(coll)
        , m_properties(std::move(properties))
        , m_secret(std::move(secret))
        , m_itemPath(std::move(itemPath))
        , m_item(existing)
        // session aliveness also need to be tracked, for potential use later in updateItem
        , m_sess(m_secret.session)
    {
    }

    DBusResult CreateItemPrompt::prompt(const DBusClientPtr& client, const QString& windowId)
    {
        if (thread() != QThread::currentThread()) {
            DBusResult ret;
            QMetaObject::invokeMethod(
                this, "prompt", Qt::BlockingQueuedConnection, Q_ARG(QString, windowId), Q_RETURN_ARG(DBusResult, ret));
            return ret;
        }

        MessageBox::OverrideParent override(findWindow(windowId));

        if (!m_coll) {
            return dismiss();
        }

        // save a weak reference to the client which may be used asynchronously later
        m_client = client;

        // the item doesn't exists yet, create it
        if (!m_item) {
            m_item = m_coll->doNewItem(client, m_itemPath);
            if (!m_item) {
                // may happen if entry somehow ends up in recycle bin
                return DBusResult(DBUS_ERROR_SECRET_NO_SUCH_OBJECT);
            }

            auto ret = updateItem();
            if (ret.err()) {
                m_item->doDelete();
                return ret;
            }
            emit completed(false, QVariant::fromValue(m_item->objectPath()));
        } else {
            bool locked = false;
            auto ret = m_item->locked(client, locked);
            if (ret.err()) {
                return ret;
            }
            if (locked) {
                // give the user a chance to unlock the item
                auto prompt = PromptBase::Create<UnlockPrompt>(service(), QSet<Collection*>{}, QSet<Item*>{m_item});
                if (!prompt) {
                    return QDBusError::InternalError;
                }
                // postpone anything after the confirmation
                connect(prompt, &PromptBase::completed, this, &CreateItemPrompt::itemUnlocked);
                return prompt->prompt(client, windowId);
            } else {
                ret = updateItem();
                if (ret.err()) {
                    return ret;
                }
                emit completed(false, QVariant::fromValue(m_item->objectPath()));
            }
        }
        return {};
    }

    DBusResult CreateItemPrompt::dismiss()
    {
        emit completed(true, QVariant::fromValue(DBusMgr::objectPathSafe(nullptr)));
        return {};
    }

    void CreateItemPrompt::itemUnlocked(bool dismissed, const QVariant& result)
    {
        auto unlocked = result.value<QList<QDBusObjectPath>>();
        if (!unlocked.isEmpty()) {
            // in theory we should check if the object path matches m_item, but a mismatch should not happen,
            // because we control the unlock prompt ourselves
            updateItem();
        }
        emit completed(dismissed, QVariant::fromValue(DBusMgr::objectPathSafe(m_item)));
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
        return {};
    }
} // namespace FdoSecrets
