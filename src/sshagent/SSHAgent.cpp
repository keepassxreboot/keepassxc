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

#include "SSHAgent.h"
#include "BinaryStream.h"
#include "KeeAgentSettings.h"

#ifndef Q_OS_WIN
#include <QtNetwork>
#else
#include <windows.h>
#endif

SSHAgent* SSHAgent::m_instance;

SSHAgent::SSHAgent(QObject* parent)
    : QObject(parent)
{
#ifndef Q_OS_WIN
    m_socketPath = QProcessEnvironment::systemEnvironment().value("SSH_AUTH_SOCK");
#endif
}

SSHAgent::~SSHAgent()
{
    for (QSet<OpenSSHKey> keys : m_keys.values()) {
        for (OpenSSHKey key : keys) {
            removeIdentity(key);
        }
    }
}

SSHAgent* SSHAgent::instance()
{
    if (m_instance == nullptr) {
        qFatal("Race condition: instance wanted before it was initialized, this is a bug.");
    }

    return m_instance;
}

void SSHAgent::init(QObject* parent)
{
    m_instance = new SSHAgent(parent);
}

const QString SSHAgent::errorString() const
{
    return m_error;
}

bool SSHAgent::isAgentRunning() const
{
#ifndef Q_OS_WIN
    return !m_socketPath.isEmpty();
#else
    return (FindWindowA("Pageant", "Pageant") != nullptr);
#endif
}

bool SSHAgent::sendMessage(const QByteArray& in, QByteArray& out)
{
#ifndef Q_OS_WIN
    QLocalSocket socket;
    BinaryStream stream(&socket);

    socket.connectToServer(m_socketPath);
    if (!socket.waitForConnected(500)) {
        m_error = tr("Agent connection failed.");
        return false;
    }

    stream.writeString(in);
    stream.flush();

    if (!stream.readString(out)) {
        m_error = tr("Agent protocol error.");
        return false;
    }

    socket.close();

    return true;
#else
    HWND hWnd = FindWindowA("Pageant", "Pageant");

    if (!hWnd) {
        m_error = tr("Agent connection failed.");
        return false;
    }

    if (static_cast<quint32>(in.length()) > AGENT_MAX_MSGLEN - 4) {
        m_error = tr("Agent connection failed.");
        return false;
    }

    QByteArray mapName =
        (QString("SSHAgentRequest") + reinterpret_cast<intptr_t>(QThread::currentThreadId())).toLatin1();

    HANDLE handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, AGENT_MAX_MSGLEN, mapName.data());

    if (!handle) {
        m_error = tr("Agent connection failed.");
        return false;
    }

    LPVOID ptr = MapViewOfFile(handle, FILE_MAP_WRITE, 0, 0, 0);

    if (!ptr) {
        CloseHandle(handle);
        m_error = tr("Agent connection failed.");
        return false;
    }

    quint32* requestLength = reinterpret_cast<quint32*>(ptr);
    void* requestData = reinterpret_cast<void*>(reinterpret_cast<char*>(ptr) + 4);

    *requestLength = qToBigEndian<quint32>(in.length());
    memcpy(requestData, in.data(), in.length());

    COPYDATASTRUCT data;
    data.dwData = AGENT_COPYDATA_ID;
    data.cbData = mapName.length() + 1;
    data.lpData = reinterpret_cast<LPVOID>(mapName.data());

    LRESULT res = SendMessageA(hWnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&data));

    if (res) {
        quint32 responseLength = qFromBigEndian<quint32>(*requestLength);
        if (responseLength <= AGENT_MAX_MSGLEN) {
            out.resize(responseLength);
            memcpy(out.data(), requestData, responseLength);
        } else {
            m_error = tr("Agent protocol error.");
        }
    } else {
        m_error = tr("Agent protocol error.");
    }

    UnmapViewOfFile(ptr);
    CloseHandle(handle);

    return (res > 0);
#endif
}

bool SSHAgent::addIdentity(OpenSSHKey& key, quint32 lifetime, bool confirm)
{
    if (!isAgentRunning()) {
        m_error = tr("No agent running, cannot add identity.");
        return false;
    }

    QByteArray requestData;
    BinaryStream request(&requestData);

    request.write((lifetime > 0 || confirm) ? SSH_AGENTC_ADD_ID_CONSTRAINED : SSH_AGENTC_ADD_IDENTITY);
    key.writePrivate(request);

    if (lifetime > 0) {
        request.write(SSH_AGENT_CONSTRAIN_LIFETIME);
        request.write(lifetime);
    }

    if (confirm) {
        request.write(SSH_AGENT_CONSTRAIN_CONFIRM);
    }

    QByteArray responseData;
    if (!sendMessage(requestData, responseData)) {
        return false;
    }

    if (responseData.length() < 1 || static_cast<quint8>(responseData[0]) != SSH_AGENT_SUCCESS) {
        m_error =
            tr("Agent refused this identity. Possible reasons include:") + "\n" + tr("The key has already been added.");

        if (lifetime > 0) {
            m_error += "\n" + tr("Restricted lifetime is not supported by the agent (check options).");
        }

        if (confirm) {
            m_error += "\n" + tr("A confirmation request is not supported by the agent (check options).");
        }

        return false;
    }

    return true;
}

bool SSHAgent::removeIdentity(OpenSSHKey& key)
{
    if (!isAgentRunning()) {
        m_error = tr("No agent running, cannot remove identity.");
        return false;
    }

    QByteArray requestData;
    BinaryStream request(&requestData);

    QByteArray keyData;
    BinaryStream keyStream(&keyData);
    key.writePublic(keyStream);

    request.write(SSH_AGENTC_REMOVE_IDENTITY);
    request.writeString(keyData);

    QByteArray responseData;
    if (!sendMessage(requestData, responseData)) {
        return false;
    }

    if (responseData.length() < 1 || static_cast<quint8>(responseData[0]) != SSH_AGENT_SUCCESS) {
        m_error = tr("Agent does not have this identity.");
        return false;
    }

    return true;
}

void SSHAgent::removeIdentityAtLock(const OpenSSHKey& key, const QUuid& uuid)
{
    OpenSSHKey copy = key;
    copy.clearPrivate();
    m_keys[uuid].insert(copy);
}

void SSHAgent::databaseModeChanged(DatabaseWidget::Mode mode)
{
    DatabaseWidget* widget = qobject_cast<DatabaseWidget*>(sender());

    if (widget == nullptr) {
        return;
    }

    const QUuid& uuid = widget->database()->uuid();

    if (mode == DatabaseWidget::LockedMode && m_keys.contains(uuid)) {

        QSet<OpenSSHKey> keys = m_keys.take(uuid);
        for (OpenSSHKey key : keys) {
            if (!removeIdentity(key)) {
                emit error(m_error);
            }
        }
    } else if (mode == DatabaseWidget::ViewMode && !m_keys.contains(uuid)) {
        for (Entry* e : widget->database()->rootGroup()->entriesRecursive()) {

            if (widget->database()->metadata()->recycleBinEnabled()
                && e->group() == widget->database()->metadata()->recycleBin()) {
                continue;
            }

            if (!e->attachments()->hasKey("KeeAgent.settings")) {
                continue;
            }

            KeeAgentSettings settings;
            settings.fromXml(e->attachments()->value("KeeAgent.settings"));

            if (!settings.allowUseOfSshKey()) {
                continue;
            }

            QByteArray keyData;
            QString fileName;
            if (settings.selectedType() == "attachment") {
                fileName = settings.attachmentName();
                keyData = e->attachments()->value(fileName);
            } else if (!settings.fileName().isEmpty()) {
                QFile file(settings.fileName());
                QFileInfo fileInfo(file);

                fileName = fileInfo.fileName();

                if (file.size() > 1024 * 1024) {
                    continue;
                }

                if (!file.open(QIODevice::ReadOnly)) {
                    continue;
                }

                keyData = file.readAll();
            }

            if (keyData.isEmpty()) {
                continue;
            }

            OpenSSHKey key;

            if (!key.parse(keyData)) {
                continue;
            }

            if (!key.openPrivateKey(e->password())) {
                continue;
            }

            if (key.comment().isEmpty()) {
                key.setComment(e->username());
            }

            if (key.comment().isEmpty()) {
                key.setComment(fileName);
            }

            if (settings.removeAtDatabaseClose()) {
                removeIdentityAtLock(key, uuid);
            }

            if (settings.addAtDatabaseOpen()) {
                int lifetime = 0;

                if (settings.useLifetimeConstraintWhenAdding()) {
                    lifetime = settings.lifetimeConstraintDuration();
                }

                if (!addIdentity(key, lifetime, settings.useConfirmConstraintWhenAdding())) {
                    emit error(m_error);
                }
            }
        }
    }
}
