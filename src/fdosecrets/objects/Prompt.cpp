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

#include "core/Tools.h"
#include "gui/DatabaseWidget.h"
#include "gui/MessageBox.h"

#include <QThread>
#include <QWindow>

namespace FdoSecrets
{

    PromptBase::PromptBase(Service* parent)
        : DBusObjectHelper(parent)
    {
        connect(this, &PromptBase::completed, this, &PromptBase::deleteLater);
    }

    bool PromptBase::registerSelf()
    {
        auto path = QStringLiteral(DBUS_PATH_TEMPLATE_PROMPT)
                        .arg(p()->objectPath().path(), Tools::uuidToHex(QUuid::createUuid()));
        bool ok = registerWithPath(path);
        if (!ok) {
            service()->plugin()->emitError(tr("Failed to register item on DBus at path '%1'").arg(path));
        }
        return ok;
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

    DBusReturn<void> PromptBase::dismiss()
    {
        emit completed(true, "");
        return {};
    }

    DBusReturn<DeleteCollectionPrompt*> DeleteCollectionPrompt::Create(Service* parent, Collection* coll)
    {
        QScopedPointer<DeleteCollectionPrompt> res{new DeleteCollectionPrompt(parent, coll)};
        if (!res->registerSelf()) {
            return DBusReturn<>::Error(QDBusError::InvalidObjectPath);
        }
        return res.take();
    }

    DeleteCollectionPrompt::DeleteCollectionPrompt(Service* parent, Collection* coll)
        : PromptBase(parent)
        , m_collection(coll)
    {
    }

    DBusReturn<void> DeleteCollectionPrompt::prompt(const QString& windowId)
    {
        if (thread() != QThread::currentThread()) {
            DBusReturn<void> ret;
            QMetaObject::invokeMethod(this,
                                      "prompt",
                                      Qt::BlockingQueuedConnection,
                                      Q_ARG(QString, windowId),
                                      Q_RETURN_ARG(DBusReturn<void>, ret));
            return ret;
        }

        if (!m_collection) {
            return DBusReturn<>::Error(QStringLiteral(DBUS_ERROR_SECRET_NO_SUCH_OBJECT));
        }

        MessageBox::OverrideParent override(findWindow(windowId));
        // only need to delete in backend, collection will react itself.
        auto accepted = service()->doCloseDatabase(m_collection->backend());

        emit completed(!accepted, "");

        return {};
    }

    DBusReturn<CreateCollectionPrompt*> CreateCollectionPrompt::Create(Service* parent)
    {
        QScopedPointer<CreateCollectionPrompt> res{new CreateCollectionPrompt(parent)};
        if (!res->registerSelf()) {
            return DBusReturn<>::Error(QDBusError::InvalidObjectPath);
        }
        return res.take();
    }

    CreateCollectionPrompt::CreateCollectionPrompt(Service* parent)
        : PromptBase(parent)
    {
    }

    DBusReturn<void> CreateCollectionPrompt::prompt(const QString& windowId)
    {
        if (thread() != QThread::currentThread()) {
            DBusReturn<void> ret;
            QMetaObject::invokeMethod(this,
                                      "prompt",
                                      Qt::BlockingQueuedConnection,
                                      Q_ARG(QString, windowId),
                                      Q_RETURN_ARG(DBusReturn<void>, ret));
            return ret;
        }

        MessageBox::OverrideParent override(findWindow(windowId));

        auto coll = service()->doNewDatabase();
        if (!coll) {
            return dismiss();
        }

        emit collectionCreated(coll);
        emit completed(false, QVariant::fromValue(coll->objectPath()));

        return {};
    }

    DBusReturn<void> CreateCollectionPrompt::dismiss()
    {
        emit completed(true, QVariant::fromValue(QDBusObjectPath{"/"}));
        return {};
    }

    DBusReturn<LockCollectionsPrompt*> LockCollectionsPrompt::Create(Service* parent, const QList<Collection*>& colls)
    {
        QScopedPointer<LockCollectionsPrompt> res{new LockCollectionsPrompt(parent, colls)};
        if (!res->registerSelf()) {
            return DBusReturn<>::Error(QDBusError::InvalidObjectPath);
        }
        return res.take();
    }

    LockCollectionsPrompt::LockCollectionsPrompt(Service* parent, const QList<Collection*>& colls)
        : PromptBase(parent)
    {
        m_collections.reserve(colls.size());
        for (const auto& c : asConst(colls)) {
            m_collections << c;
        }
    }

    DBusReturn<void> LockCollectionsPrompt::prompt(const QString& windowId)
    {
        if (thread() != QThread::currentThread()) {
            DBusReturn<void> ret;
            QMetaObject::invokeMethod(this,
                                      "prompt",
                                      Qt::BlockingQueuedConnection,
                                      Q_ARG(QString, windowId),
                                      Q_RETURN_ARG(DBusReturn<void>, ret));
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

    DBusReturn<void> LockCollectionsPrompt::dismiss()
    {
        emit completed(true, QVariant::fromValue(m_locked));
        return {};
    }

    DBusReturn<UnlockCollectionsPrompt*> UnlockCollectionsPrompt::Create(Service* parent,
                                                                         const QList<Collection*>& coll)
    {
        QScopedPointer<UnlockCollectionsPrompt> res{new UnlockCollectionsPrompt(parent, coll)};
        if (!res->registerSelf()) {
            return DBusReturn<>::Error(QDBusError::InvalidObjectPath);
        }
        return res.take();
    }

    UnlockCollectionsPrompt::UnlockCollectionsPrompt(Service* parent, const QList<Collection*>& colls)
        : PromptBase(parent)
    {
        m_collections.reserve(colls.size());
        for (const auto& c : asConst(colls)) {
            m_collections << c;
        }
    }

    DBusReturn<void> UnlockCollectionsPrompt::prompt(const QString& windowId)
    {
        if (thread() != QThread::currentThread()) {
            DBusReturn<void> ret;
            QMetaObject::invokeMethod(this,
                                      "prompt",
                                      Qt::BlockingQueuedConnection,
                                      Q_ARG(QString, windowId),
                                      Q_RETURN_ARG(DBusReturn<void>, ret));
            return ret;
        }

        MessageBox::OverrideParent override(findWindow(windowId));

        for (const auto& c : asConst(m_collections)) {
            if (c) {
                // doUnlock is nonblocking
                connect(c, &Collection::doneUnlockCollection, this, &UnlockCollectionsPrompt::collectionUnlockFinished);
                c->doUnlock();
            }
        }

        return {};
    }

    void UnlockCollectionsPrompt::collectionUnlockFinished(bool accepted)
    {
        auto coll = qobject_cast<Collection*>(sender());
        if (!coll) {
            return;
        }

        if (!m_collections.contains(coll)) {
            // should not happen
            coll->disconnect(this);
            return;
        }

        // one shot
        coll->disconnect(this);

        if (accepted) {
            m_unlocked << coll->objectPath();
        } else {
            m_numRejected += 1;
        }

        // if we've get all
        if (m_numRejected + m_unlocked.size() == m_collections.size()) {
            emit completed(m_unlocked.isEmpty(), QVariant::fromValue(m_unlocked));
        }
    }
    DBusReturn<void> UnlockCollectionsPrompt::dismiss()
    {
        emit completed(true, QVariant::fromValue(m_unlocked));
        return {};
    }

    DBusReturn<DeleteItemPrompt*> DeleteItemPrompt::Create(Service* parent, Item* item)
    {
        QScopedPointer<DeleteItemPrompt> res{new DeleteItemPrompt(parent, item)};
        if (!res->registerSelf()) {
            return DBusReturn<>::Error(QDBusError::InvalidObjectPath);
        }
        return res.take();
    }

    DeleteItemPrompt::DeleteItemPrompt(Service* parent, Item* item)
        : PromptBase(parent)
        , m_item(item)
    {
    }

    DBusReturn<void> DeleteItemPrompt::prompt(const QString& windowId)
    {
        if (thread() != QThread::currentThread()) {
            DBusReturn<void> ret;
            QMetaObject::invokeMethod(this,
                                      "prompt",
                                      Qt::BlockingQueuedConnection,
                                      Q_ARG(QString, windowId),
                                      Q_RETURN_ARG(DBusReturn<void>, ret));
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
