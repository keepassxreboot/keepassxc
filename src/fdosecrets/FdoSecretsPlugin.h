/*
 *  Copyright (C) 2018 Aetf <aetf@unlimitedcodeworks.xyz>
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

#ifndef KEEPASSXC_FDOSECRETSPLUGIN_H
#define KEEPASSXC_FDOSECRETSPLUGIN_H

#include "core/Global.h"
#include <QPointer>

class Database;
class Entry;
class FdoSecretsPluginRequestHandler;

namespace FdoSecrets
{
    class Collection;
    class DBusClient;
    class DBusMgr;
    class Service;
    class UnlockPrompt;
    class PromptBase;
} // namespace FdoSecrets

class FdoSecretsPlugin : public QObject
{
    Q_OBJECT

    friend class FdoSecrets::Collection;
    friend class FdoSecrets::Service;
    friend class FdoSecrets::UnlockPrompt;
    friend class FdoSecrets::PromptBase;

public:
    FdoSecretsPlugin(QObject* parent);
    ~FdoSecretsPlugin() override = default;

    void updateServiceState();

    /**
     * @return The service instance, can be nullptr if the service is disabled.
     */
    FdoSecrets::Service* serviceInstance() const;

    /**
     * @brief The D-Bus manager instance
     * @return
     */
    const QSharedPointer<FdoSecrets::DBusMgr>& dbus() const;

public slots:
    void emitRequestShowNotification(const QString& msg, const QString& title = {});

    void registerDatabase(const QString& name);
    void unregisterDatabase(const QString& name);

    void databaseLocked(const QString& name);
    void databaseUnlocked(const QString& name, QSharedPointer<Database> db);

    /**
     * @brief Show error in the UI
     * @param msg
     */
    void emitError(const QString& msg);

signals:
    void error(const QString& msg);
    void requestShowNotification(const QString& msg, const QString& title, int msTimeoutHint);
    void secretServiceStarted();
    void secretServiceStopped();

private:
    virtual size_t requestEntriesRemove(const QSharedPointer<FdoSecrets::DBusClient>& client,
                                        const QString& name,
                                        const QList<Entry*>& entries,
                                        bool permanent) const = 0;

    virtual bool requestEntriesUnlock(const QSharedPointer<FdoSecrets::DBusClient>& client,
                                      const QString& windowId,
                                      const QList<Entry*>& entries,
                                      QHash<Entry*, AuthDecision>& decisions,
                                      AuthDecision& forFutureEntries) const = 0;

    virtual bool doLockDatabase(const QSharedPointer<FdoSecrets::DBusClient>& client, const QString& name) = 0;
    virtual bool doUnlockDatabase(const QSharedPointer<FdoSecrets::DBusClient>& client, const QString& name) = 0;

    virtual bool requestUnlockAnyDatabase(const QSharedPointer<FdoSecrets::DBusClient>& client) const = 0;
    virtual QString requestNewDatabase(const QSharedPointer<FdoSecrets::DBusClient>& client) = 0;

    virtual QString overrideMessageBoxParent(const QString& windowId) const = 0;

private:
    QSharedPointer<FdoSecrets::DBusMgr> m_dbus;
    QSharedPointer<FdoSecrets::Service> m_secretService;
};

#endif // KEEPASSXC_FDOSECRETSPLUGIN_H
