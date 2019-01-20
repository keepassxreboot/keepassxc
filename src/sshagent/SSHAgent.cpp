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

#include "core/Config.h"
#include "crypto/ssh/BinaryStream.h"
#include "crypto/ssh/OpenSSHKey.h"
#include "sshagent/KeeAgentSettings.h"

#include <QtNetwork>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

SSHAgent* SSHAgent::m_instance;

SSHAgent::SSHAgent(QObject* parent)
    : QObject(parent)
{
#ifndef Q_OS_WIN
    m_socketPath = QProcessEnvironment::systemEnvironment().value("SSH_AUTH_SOCK");
#else
    m_socketPath = "\\\\.\\pipe\\openssh-ssh-agent";
#endif
}

SSHAgent::~SSHAgent()
{
    auto it = m_addedKeys.begin();
    while (it != m_addedKeys.end()) {
        OpenSSHKey key = it.key();
        removeIdentity(key);
        it = m_addedKeys.erase(it);
    }
}

SSHAgent* SSHAgent::instance()
{
    if (!m_instance) {
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
    if (!config()->get("SSHAgentOpenSSH").toBool()) {
        return (FindWindowA("Pageant", "Pageant") != nullptr);
    } else {
        return WaitNamedPipe(m_socketPath.toLatin1().data(), 100);
    }
#endif
}

bool SSHAgent::sendMessage(const QByteArray& in, QByteArray& out)
{
#ifdef Q_OS_WIN
    if (!config()->get("SSHAgentOpenSSH").toBool()) {
        return sendMessagePageant(in, out);
    }
#endif

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
}

#ifdef Q_OS_WIN
bool SSHAgent::sendMessagePageant(const QByteArray& in, QByteArray& out)
{
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
}
#endif

/**
 * Add the identity to the SSH agent.
 *
 * @param key identity / key to add
 * @param lifetime time after which the key should expire
 * @param confirm ask for confirmation before adding the key
 * @param removeOnLock autoremove from agent when the Database is locked
 * @return true on success
 */
bool SSHAgent::addIdentity(OpenSSHKey& key, bool removeOnLock, quint32 lifetime, bool confirm)
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

    OpenSSHKey keyCopy = key;
    keyCopy.clearPrivate();
    m_addedKeys[keyCopy] = removeOnLock;
    return true;
}

/**
 * Remove an identity from the SSH agent.
 *
 * @param key identity to remove
 * @return true on success
 */
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
    return sendMessage(requestData, responseData);
}

/**
 * Change "remove identity on lock" setting for a key already added to the agent.
 * Will to nothing if the key has not been added to the agent.
 *
 * @param key key to change setting for
 * @param autoRemove whether to remove the key from the agent when database is locked
 */
void SSHAgent::setAutoRemoveOnLock(const OpenSSHKey& key, bool autoRemove)
{
    if (m_addedKeys.contains(key)) {
        m_addedKeys[key] = autoRemove;
    }
}

void SSHAgent::databaseModeChanged()
{
    auto* widget = qobject_cast<DatabaseWidget*>(sender());
    if (!widget) {
        return;
    }

    if (widget->isLocked()) {
        auto it = m_addedKeys.begin();
        while (it != m_addedKeys.end()) {
            OpenSSHKey key = it.key();
            if (it.value()) {
                if (!removeIdentity(key)) {
                    emit error(m_error);
                }
                it = m_addedKeys.erase(it);
            } else {
                // don't remove it yet
                m_addedKeys[key] = false;
                ++it;
            }
        }

        return;
    }

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

        if (!key.parsePKCS1PEM(keyData)) {
            continue;
        }

        if (!key.openKey(e->password())) {
            continue;
        }

        if (key.comment().isEmpty()) {
            key.setComment(e->username());
        }

        if (key.comment().isEmpty()) {
            key.setComment(fileName);
        }

        if (!m_addedKeys.contains(key) && settings.addAtDatabaseOpen()) {
            quint32 lifetime = 0;

            if (settings.useLifetimeConstraintWhenAdding()) {
                lifetime = static_cast<quint32>(settings.lifetimeConstraintDuration());
            }

            if (!addIdentity(
                    key, settings.removeAtDatabaseClose(), lifetime, settings.useConfirmConstraintWhenAdding())) {
                emit error(m_error);
            }
        }
    }
}
