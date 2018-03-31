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

#include "BrowserClients.h"
#include <QJsonParseError>
#include <QJsonValue>

BrowserClients::BrowserClients(BrowserService& browserService)
    : m_mutex(QMutex::Recursive)
    , m_browserService(browserService)
{
    m_clients.reserve(1000);
}

QJsonObject BrowserClients::readResponse(const QByteArray& arr)
{
    QJsonObject json;
    const QJsonObject message = byteArrayToJson(arr);
    const QString clientID = getClientID(message);

    if (!clientID.isEmpty()) {
        const ClientPtr client = getClient(clientID);
        if (client->browserAction) {
            json = client->browserAction->readResponse(message);
        }
    }

    return json;
}

QJsonObject BrowserClients::byteArrayToJson(const QByteArray& arr) const
{
    QJsonObject json;
    QJsonParseError err;
    QJsonDocument doc(QJsonDocument::fromJson(arr, &err));
    if (doc.isObject()) {
        json = doc.object();
    }

    return json;
}

QString BrowserClients::getClientID(const QJsonObject& json) const
{
    return json["clientID"].toString();
}

BrowserClients::ClientPtr BrowserClients::getClient(const QString& clientID)
{
    QMutexLocker locker(&m_mutex);
    for (const auto& i : m_clients) {
        if (i->clientID.compare(clientID, Qt::CaseSensitive) == 0) {
            return i;
        }
    }

    // clientID not found, create a new client
    QSharedPointer<BrowserAction> ba = QSharedPointer<BrowserAction>::create(m_browserService);
    ClientPtr client = ClientPtr::create(clientID, ba);
    m_clients.push_back(client);
    return m_clients.back();
}
