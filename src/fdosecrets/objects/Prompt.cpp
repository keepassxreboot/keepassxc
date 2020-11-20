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
            QMetaObject::invokeMethod(this,
                                      "prompt",
                                      Qt::BlockingQueuedConnection,
                                      Q_ARG(QString, windowId),
                                      Q_RETURN_ARG(DBusResult, ret));
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
            QMetaObject::invokeMethod(this,
                                      "prompt",
                                      Qt::BlockingQueuedConnection,
                                      Q_ARG(QString, windowId),
                                      Q_RETURN_ARG(DBusResult, ret));
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
            QMetaObject::invokeMethod(this,
                                      "prompt",
                                      Qt::BlockingQueuedConnection,
                                      Q_ARG(QString, windowId),
                                      Q_RETURN_ARG(DBusResult, ret));
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

    UnlockCollectionsPrompt::UnlockCollectionsPrompt(Service* parent, const QList<Collection*>& colls)
        : PromptBase(parent)
    {
        m_collections.reserve(colls.size());
        for (const auto& c : asConst(colls)) {
            m_collections << c;
        }
    }

    DBusResult UnlockCollectionsPrompt::prompt(const QString& windowId)
    {
        if (thread() != QThread::currentThread()) {
            DBusResult ret;
            QMetaObject::invokeMethod(this,
                                      "prompt",
                                      Qt::BlockingQueuedConnection,
                                      Q_ARG(QString, windowId),
                                      Q_RETURN_ARG(DBusResult, ret));
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

    DBusResult UnlockCollectionsPrompt::dismiss()
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
            QMetaObject::invokeMethod(this,
                                      "prompt",
                                      Qt::BlockingQueuedConnection,
                                      Q_ARG(QString, windowId),
                                      Q_RETURN_ARG(DBusResult, ret));
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
