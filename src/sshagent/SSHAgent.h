/*
 *  Copyright (C) 2017 Toni Spets <toni.spets@iki.fi>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef AGENTCLIENT_H
#define AGENTCLIENT_H

#include "OpenSSHKey.h"
#include <QList>
#include <QtCore>

#include "gui/DatabaseWidget.h"

class SSHAgent : public QObject
{
    Q_OBJECT

public:
    static SSHAgent* instance();
    static void init(QObject* parent);

    const QString errorString() const;
    bool isAgentRunning() const;
    bool addIdentity(OpenSSHKey& key, quint32 lifetime = 0, bool confirm = false);
    bool removeIdentity(OpenSSHKey& key);
    void removeIdentityAtLock(const OpenSSHKey& key, const QUuid& uuid);

signals:
    void error(const QString& message);

public slots:
    void databaseModeChanged(DatabaseWidget::Mode mode = DatabaseWidget::LockedMode);

private:
    const quint8 SSH_AGENT_FAILURE = 5;
    const quint8 SSH_AGENT_SUCCESS = 6;
    const quint8 SSH_AGENTC_REQUEST_IDENTITIES = 11;
    const quint8 SSH_AGENT_IDENTITIES_ANSWER = 12;
    const quint8 SSH_AGENTC_ADD_IDENTITY = 17;
    const quint8 SSH_AGENTC_REMOVE_IDENTITY = 18;
    const quint8 SSH_AGENTC_ADD_ID_CONSTRAINED = 25;

    const quint8 SSH_AGENT_CONSTRAIN_LIFETIME = 1;
    const quint8 SSH_AGENT_CONSTRAIN_CONFIRM = 2;

    explicit SSHAgent(QObject* parent = nullptr);
    ~SSHAgent();

    bool sendMessage(const QByteArray& in, QByteArray& out);

    static SSHAgent* m_instance;

#ifndef Q_OS_WIN
    QString m_socketPath;
#else
    const quint32 AGENT_MAX_MSGLEN = 8192;
    const quint32 AGENT_COPYDATA_ID = 0x804e50ba;
#endif

    QMap<QUuid, QSet<OpenSSHKey>> m_keys;
    QString m_error;
};

#endif // AGENTCLIENT_H
