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

#include "fdosecrets/FdoSecretsSettings.h"
#include "fdosecrets/objects/Collection.h"
#include "fdosecrets/objects/Connection.h"
#include "fdosecrets/objects/Item.h"
#include "fdosecrets/objects/Service.h"

#include "core/Tools.h"
#include "gui/DatabaseWidget.h"
#include "gui/MessageBox.h"

#include <QCheckBox>
#include <QThread>
#include <QWindow>

namespace FdoSecrets
{

    PromptBase::PromptBase(Service* parent)
        : DBusObject(parent)
    {
        registerWithPath(
            QStringLiteral("%1/prompt/%2").arg(p()->objectPath().path(), Tools::uuidToHex(QUuid::createUuid())),
            new PromptAdaptor(this));

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

    DBusReturn<void> PromptBase::dismiss()
    {
        emit completed(true, "");
        return {};
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

    UnlockCollectionsPrompt::UnlockCollectionsPrompt(Service* parent,
                                                     Connection* conn,
                                                     const QSet<Collection*>& colls,
                                                     const QSet<Item*>& items)
        : PromptBase(parent)
        , m_conn(conn)
    {
        m_collections.reserve(colls.size());
        for (const auto& c : asConst(colls)) {
            m_collections << c;
        }
        m_items.reserve(items.size());
        for (const auto& i : items) {
            m_items << i;
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

        // This only initiates the unlock but does not block. Items are unlocked after all collections have been
        // unlocked to prevent multiple simultaneous dialogues.
        for (const auto& c : asConst(m_collections)) {
            if (c) {
                // doUnlock is nonblocking
                connect(c, &Collection::doneUnlockCollection, this, &UnlockCollectionsPrompt::collectionUnlockFinished);
                c->doUnlock();
            }
        }

        // Directly handle items if no collections need to be unlocked.
        if (m_collections.isEmpty()) {
            unlockItems();
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
            if (m_unlocked.isEmpty()) {
                emit completed(true, QVariant::fromValue(m_unlocked));
            } else {
                unlockItems();
            }
        }
    }
    DBusReturn<void> UnlockCollectionsPrompt::dismiss()
    {
        emit completed(true, QVariant::fromValue(m_unlocked));
        return {};
    }

    void UnlockCollectionsPrompt::unlockItems()
    {
        if (m_conn && m_conn->connected() && !m_items.empty()) {
            QString q =
                tr("The application '%1' requested access to the following entries:\n\n").arg(m_conn->peerName());
            for (const auto& i : m_items) {
                if (i) {
                    q += QStringLiteral("%1 %2\n").arg(QChar(0x2022)).arg(i->backend()->title());
                }
            }
            q += tr("\nAttention: the authenticity of the application name cannot be guaranteed!\n\n");
            q += tr("Do you allow access to these entries only, all entries or deny the access?");

            auto* checkbox = new QCheckBox(tr("Always request confirmations when applications access secrets"));
            checkbox->setChecked(true);
            checkbox->setToolTip(tr("You can change this setting from \"Settings\" -> \"Secret-Service-Integration\" "
                                    "-> \"Confirm when passwords are retrieved by clients\" too."));
            QObject::connect(checkbox, &QCheckBox::stateChanged, [&](int state) {
                FdoSecrets::settings()->setConfirmAccessItem(static_cast<Qt::CheckState>(state)
                                                             == Qt::CheckState::Checked);
            });
            auto dialogResult = MessageBox::question(nullptr,
                                                     tr("KeePassXC: Secret-service access requested"),
                                                     q,
                                                     MessageBox::Yes | MessageBox::YesToAll | MessageBox::No,
                                                     MessageBox::NoButton,
                                                     MessageBox::Raise,
                                                     checkbox);

            if (m_conn && m_conn->connected()) {
                if (dialogResult == MessageBox::Yes) {
                    for (const auto& i : m_items) {
                        if (i) {
                            m_conn->setAuthorizedItem(i->backend()->uuid());
                            m_unlocked << i->objectPath();
                        }
                    }
                } else if (dialogResult == MessageBox::YesToAll) {
                    m_conn->setAuthorizedAll();
                    for (const auto& i : m_items) {
                        if (i) {
                            m_unlocked << i->objectPath();
                        }
                    }
                }
            }
        }

        // if we've get all
        emit completed(m_unlocked.isEmpty(), QVariant::fromValue(m_unlocked));
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

    OverwritePrompt::OverwritePrompt(Service* parent,
                                     Connection* conn,
                                     Item* item,
                                     const QVariantMap& properties,
                                     const SecretStruct& secret)
        : PromptBase(parent)
        , m_conn(conn)
        , m_item(item)
        , m_properties(properties)
        , m_secret(secret)
    {
    }

    DBusReturn<void> OverwritePrompt::prompt(const QString& windowId)
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

        bool dismissed;
        if (m_conn && m_conn->connected() && m_item) {
            auto l = m_item->locked(m_conn);
            if (l.isError()) {
                dismissed = true;
            } else if (!l.value()) {
                // not locked anymore
                dismissed = false;
            } else {
                QString q = tr("An application wants to the update the following entry: %1\n\n")
                                .arg(m_item->backend()->title());
                q += tr("\nApplication: %1\n\n").arg(m_conn->peerName());
                q += tr("Do you allow access to this entry only, all entries or deny the access?");

                auto* checkbox = new QCheckBox(tr("Always request confirmations when applications access secrets"));
                checkbox->setChecked(true);
                checkbox->setToolTip(
                    tr("You can change this setting from \"Settings\" -> \"Secret-Service-Integration\" -> \"Confirm "
                       "when passwords are retrieved by clients\" too."));
                QObject::connect(checkbox, &QCheckBox::stateChanged, [&](int state) {
                    FdoSecrets::settings()->setConfirmAccessItem(static_cast<Qt::CheckState>(state)
                                                                 == Qt::CheckState::Checked);
                });
                auto dialogResult = MessageBox::question(nullptr,
                                                         tr("KeePassXC: Secret-service access requested"),
                                                         q,
                                                         MessageBox::Yes | MessageBox::YesToAll | MessageBox::No,
                                                         MessageBox::NoButton,
                                                         MessageBox::Raise,
                                                         checkbox);

                if (dialogResult == MessageBox::Yes) {
                    m_conn->setAuthorizedItem(m_item->backend()->uuid());
                    dismissed = false;
                } else if (dialogResult == MessageBox::YesToAll) {
                    m_conn->setAuthorizedAll();
                    dismissed = false;
                } else {
                    dismissed = true;
                }

                if (!dismissed) {
                    dismissed = m_item->setSecret(m_secret, m_conn).isError();
                }
                if (!dismissed) {
                    dismissed = m_item->setProperties(m_properties).isError();
                }
            }
        } else {
            dismissed = true;
        }

        emit completed(dismissed, dismissed ? QVariant() : QVariant::fromValue(objectPathSafe(m_item.data())));
        return {};
    }

} // namespace FdoSecrets
