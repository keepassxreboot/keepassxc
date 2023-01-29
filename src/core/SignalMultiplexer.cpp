/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "SignalMultiplexer.h"

#include "core/Global.h"

SignalMultiplexer::SignalMultiplexer() = default;

SignalMultiplexer::~SignalMultiplexer()
{
    // disconnect all connections
    setCurrentObject(nullptr);
}

QObject* SignalMultiplexer::currentObject() const
{
    return m_currentObject;
}

void SignalMultiplexer::setCurrentObject(QObject* object)
{
    // remove all Connections from the list whose senders/receivers have been deleted
    QMutableListIterator<Connection> i = m_connections;
    while (i.hasNext()) {
        const Connection& con = i.next();

        if (!con.sender && !con.receiver) {
            i.remove();
        }
    }

    if (m_currentObject) {
        for (const Connection& con : asConst(m_connections)) {
            disconnect(con);
        }
    }

    m_currentObject = object;

    if (object) {
        for (const Connection& con : asConst(m_connections)) {
            connect(con);
        }
    }
}

void SignalMultiplexer::connect(QObject* sender, const char* signal, const char* slot)
{
    Q_ASSERT(sender);

    Connection con;
    con.slot = slot;
    con.sender = sender;
    con.signal = signal;
    m_connections << con;

    if (m_currentObject) {
        connect(con);
    }
}

void SignalMultiplexer::connect(const char* signal, QObject* receiver, const char* slot)
{
    Q_ASSERT(receiver);

    Connection con;
    con.receiver = receiver;
    con.signal = signal;
    con.slot = slot;
    m_connections << con;

    if (m_currentObject) {
        connect(con);
    }
}

void SignalMultiplexer::disconnect(QObject* sender, const char* signal, const char* slot)
{
    Q_ASSERT(sender);

    QMutableListIterator<Connection> i = m_connections;
    while (i.hasNext()) {
        const Connection& con = i.next();

        if (con.sender == sender && qstrcmp(con.signal, signal) == 0 && qstrcmp(con.slot, slot) == 0) {
            if (m_currentObject) {
                disconnect(con);
            }
            i.remove();
        }
    }
}

void SignalMultiplexer::disconnect(const char* signal, QObject* receiver, const char* slot)
{
    Q_ASSERT(receiver);

    QMutableListIterator<Connection> i = m_connections;
    while (i.hasNext()) {
        const Connection& con = i.next();

        if (con.receiver == receiver && qstrcmp(con.signal, signal) == 0 && qstrcmp(con.slot, slot) == 0) {
            if (m_currentObject) {
                disconnect(con);
            }
            i.remove();
        }
    }
}

void SignalMultiplexer::connect(const Connection& con)
{
    Q_ASSERT(con.sender || con.receiver);

    if (con.sender) {
        QObject::connect(con.sender, con.signal, m_currentObject, con.slot);
    } else {
        QObject::connect(m_currentObject, con.signal, con.receiver, con.slot);
    }
}

void SignalMultiplexer::disconnect(const Connection& con)
{
    Q_ASSERT(con.sender || con.receiver);

    if (con.sender) {
        QObject::disconnect(con.sender, con.signal, m_currentObject, con.slot);
    } else {
        QObject::disconnect(m_currentObject, con.signal, con.receiver, con.slot);
    }
}
