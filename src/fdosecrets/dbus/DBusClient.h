/*
 *  Copyright (C) 2020 Aetf <aetf@unlimitedcode.works>
 *  Copyright (C) 2020 Jan Klötzke <jan@kloetzke.net>
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

#ifndef KEEPASSXC_FDOSECRETS_DBUSCLIENT_H
#define KEEPASSXC_FDOSECRETS_DBUSCLIENT_H

#include <QPointer>
#include <QSet>
#include <QUuid>

#include "core/Global.h"

namespace FdoSecrets
{
    class DBusMgr;
    class CipherPair;

    struct ProcInfo
    {
        uint pid;
        uint ppid;
        QString exePath;
        QString name;
        QString command;

        bool operator==(const ProcInfo& other) const;
        bool operator!=(const ProcInfo& other) const;
    };

    /**
     * Contains info representing a process.
     * This can be obtained by DBusMgr::serviceInfo given a dbus address.
     */
    struct PeerInfo
    {
        /**
         * @brief DBus address
         */
        QString address;

        uint pid;
        /**
         * @brief Whether current process' exePath points to a valid executable file.
         *
         * Note that an empty exePath is not valid.
         */
        bool valid;

        /**
         * @brief List of parents of the process.
         *
         * The first element is the current process. The last element is usually PID 1.
         *
         * This is for showing to the user only and is intentionally simple.
         * Getting detailed process info is beyond the scope of KPXC.
         */
        QList<ProcInfo> hierarchy;

        QString exePath() const
        {
            return hierarchy.front().exePath;
        }

        bool operator==(const PeerInfo& other) const;
        bool operator!=(const PeerInfo& other) const;
    };

    /**
     * Represent a client that has made requests to our service. A client is identified by its
     * DBus address, which is guaranteed to be unique by the DBus protocol.
     *
     * An object of this class is created on the first request and destroyed
     * when the client address vanishes from the bus. DBus guarantees that the
     * client address is not reused.
     *
     * One client may have multiple `Session`s with our service, and this class
     * manages the negotiation state (if any) of ciphers and per-client authorization
     * status.
     */
    class DBusClient
    {
    public:
        /**
         * @brief Given peer's service address, construct a client object
         * @param address obtained from `QDBusMessage::service()`
         * @param process the process info
         */
        explicit DBusClient(DBusMgr* dbus, PeerInfo process);

        DBusMgr* dbus() const;

        /**
         * @return The human readable client name, usually the process name
         */
        QString name() const;

        /**
         * @return The unique DBus address of the client
         */
        QString address() const
        {
            return m_process.address;
        }

        /**
         * @return The process id of the client
         */
        uint pid() const
        {
            return m_process.pid;
        }

        /**
         * @return The process info
         */
        const PeerInfo& processInfo() const
        {
            return m_process;
        }

        QSharedPointer<CipherPair>
        negotiateCipher(const QString& algorithm, const QVariant& input, QVariant& output, bool& incomplete);

        /**
         * Check if the item is known in this client's auth list
         */
        bool itemKnown(const QUuid& uuid) const;

        /**
         * Check if client may access item identified by @a uuid.
         */
        bool itemAuthorized(const QUuid& uuid) const;

        /**
         * Check if client may access item identified by @a uuid, and also reset any once auth.
         */
        bool itemAuthorizedResetOnce(const QUuid& uuid);

        /**
         * Authorize client to access item identified by @a uuid.
         */
        void setItemAuthorized(const QUuid& uuid, AuthDecision auth);

        /**
         * Authorize client to access all items.
         */
        void setAllAuthorized(AuthDecision authorized);

        /**
         * Forget all previous authorization.
         */
        void clearAuthorization();

        /**
         * Forcefully disconnect the client.
         * Force close any remaining session, and cleanup negotiation states
         */
        void disconnectDBus();

    private:
        QPointer<DBusMgr> m_dbus;
        PeerInfo m_process;

        AuthDecision m_authorizedAll{AuthDecision::Undecided};

        QSet<QUuid> m_allowed{};
        QSet<QUuid> m_denied{};

        QSet<QUuid> m_allowedOnce{};
        QSet<QUuid> m_deniedOnce{};
    };

    using DBusClientPtr = QSharedPointer<DBusClient>;
} // namespace FdoSecrets
Q_DECLARE_METATYPE(FdoSecrets::DBusClientPtr);

#endif // KEEPASSXC_FDOSECRETS_DBUSCLIENT_H
