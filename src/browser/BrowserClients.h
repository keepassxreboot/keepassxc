/*
*  Copyright (C) 2017 Sami Vänttinen <sami.vanttinen@protonmail.com>
*  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BROWSERCLIENTS_H
#define BROWSERCLIENTS_H

#include "BrowserAction.h"
#include <QJsonObject>
#include <QLocalSocket>
#include <QMutex>
#include <QSharedPointer>
#include <QVector>

class BrowserClients
{
    struct Client
    {
        Client(const QString& id, QSharedPointer<BrowserAction> ba)
            : clientID(id)
            , browserAction(ba)
        {
        }
        QString clientID;
        QSharedPointer<BrowserAction> browserAction;
    };

    typedef QSharedPointer<Client> ClientPtr;

public:
    BrowserClients(BrowserService& browserService);
    ~BrowserClients() = default;

    QJsonObject readResponse(const QByteArray& arr);

private:
    QJsonObject byteArrayToJson(const QByteArray& arr) const;
    QString getClientID(const QJsonObject& json) const;
    ClientPtr getClient(const QString& clientID);

private:
    QMutex m_mutex;
    QVector<ClientPtr> m_clients;
    BrowserService& m_browserService;
};

#endif // BROWSERCLIENTS_H
