/*
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

#ifndef KEEPASSXC_FDOSECRETS_CONNECTION_H
#define KEEPASSXC_FDOSECRETS_CONNECTION_H

#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QUuid>

namespace FdoSecrets
{
    class Service;
    class Session;

    /**
     * Represent a client that has made requests to our service.
     *
     * The DBus model does not have any notion of a connection between client
     * and service. This implies that we cannot disconnect a client but every
     * request handler must check the connected() property if a request should
     * be handled or not.
     *
     * A object of this class is created on the first request and destroyed
     * when the client address vanishes from the bus. DBus guarantees that the
     * client address is not reused.
     */
    class Connection : public QObject
    {
        Q_OBJECT
    public:
        explicit Connection(Service* parent, const QString& peerName);
        ~Connection() override;

        /**
         * The human readable peer name.
         */
        QString peerName() const
        {
            return m_peerName;
        }

        /**
         * Property signaling whether requests should be handled.
         *
         * If false a client request should be rejected.
         */
        bool connected() const
        {
            return m_connected;
        }

        /**
         * Add Session object to the client state.
         *
         * Will take the ownership of the @a sess object.
         */
        void addSession(Session* sess);

        /**
         * Check if client may access item identified by @a uuid.
         */
        bool authorized(const QUuid& uuid) const;

        /**
         * Authorize client to access all items.
         */
        void setAuthorizedAll();

        /**
         * Authorize client to access item identified by @a uuid.
         */
        void setAuthorizedItem(const QUuid& uuid);

    public slots:
        /**
         * Forcefully disconnect the client.
         */
        void disconnectDBus();

    signals:
        void disconnectedDBus();

    private:
        QString m_peerName;
        bool m_connected{true};
        bool m_authorizedAll{false};
        QSet<QUuid> m_authorizedEntries{};
        QList<Session*> m_sessions;
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_CONNECTION_H
