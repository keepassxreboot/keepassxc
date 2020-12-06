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
#include "fdosecrets/FdoSecretsSettings.h"
#include "fdosecrets/objects/Collection.h"
#include "fdosecrets/objects/Item.h"
#include "fdosecrets/objects/Service.h"
#include "fdosecrets/widgets/AccessControlDialog.h"

#include "core/Tools.h"
#include "gui/DatabaseWidget.h"
#include "gui/MessageBox.h"

#include <QThread>
#include <QWindow>
#include <utility>

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

    DBusResult DeleteCollectionPrompt::prompt(const QString& windowId)
    {
        if (thread() != QThread::currentThread()) {
            DBusResult ret;
            QMetaObject::invokeMethod(
                this, "prompt", Qt::BlockingQueuedConnection, Q_ARG(QString, windowId), Q_RETURN_ARG(DBusResult, ret));
            return ret;
        }

        if (!m_collection) {
            return DBusResult(QStringLiteral(DBUS_ERROR_SECRET_NO_SUCH_OBJECT));
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

    DBusResult CreateCollectionPrompt::prompt(const QString& windowId)
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
        emit completed(true, QVariant::fromValue(QDBusObjectPath{"/"}));
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

    DBusResult LockCollectionsPrompt::prompt(const QString& windowId)
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

    DBusResult UnlockPrompt::prompt(const QString& windowId)
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
        m_client = dbus().callingClient();

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
            unlockItems();
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

        // flatten to list of items
        QList<Item*> items;
        for (const auto& itemsPerColl : m_items.values()) {
            for (const auto& item : itemsPerColl) {
                if (item) {
                    items << item;
                }
            }
        }
        QList<Entry*> entries;
        entries.reserve(m_items.size());
        for (const auto& item : asConst(items)) {
            entries << item->backend();
        }
        if (!entries.isEmpty()) {
            AccessControlDialog ac(findWindow(m_windowId), entries, client->name(), client->pid());
            switch (ac.exec()) {
            case AccessControlDialog::AllowAll:
                client->setAllAuthorized(true);
                for (const auto& item : asConst(items)) {
                    m_unlocked << item->objectPath();
                }
                break;
            case AccessControlDialog::AllowSelected:
                for (const auto& idx : ac.getEntryIndices()) {
                    const auto& item = items.at(idx);
                    client->setItemAuthorized(item->backend()->uuid());
                    m_unlocked << item->objectPath();
                }
                break;
            case AccessControlDialog::DenyAll:
            default:
                dismiss();
                return;
            }
        }
        emit completed(m_unlocked.isEmpty(), QVariant::fromValue(m_unlocked));
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

    DBusResult DeleteItemPrompt::prompt(const QString& windowId)
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
            if (FdoSecrets::settings()->noConfirmDeleteItem()) {
                // based on permanent or not, different button is used
                if (m_item->isDeletePermanent()) {
                    MessageBox::setNextAnswer(MessageBox::Delete);
                } else {
                    MessageBox::setNextAnswer(MessageBox::Move);
                }
            }
            m_item->collection()->doDeleteEntries({m_item->backend()});
        }

        emit completed(false, "");

        return {};
    }
} // namespace FdoSecrets
