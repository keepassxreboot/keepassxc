/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#include "BrowserClientRestrictions.h"
#include "core/DatabaseSettings.h"
#include "core/FileHash.h"
#include "core/Metadata.h"
#include <QCryptographicHash>

#ifdef Q_OS_MACOS
#include <libproc.h>
#include <sys/un.h>
#elif Q_OS_LINUX
#include <limits.h>
#elif Q_OS_WIN
#include <psapi.h>
#elif Q_OS_UNIX

#endif

bool BrowserClientRestrictions::isClientProcessAllowed(const QSharedPointer<Database>& db,
                                                       const ClientProcess& clientProcess)
{
    for (const auto& key : db->metadata()->customData()->keys()) {
        if (key.startsWith(CustomData::OptionPrefix + CustomData::BrowserAllowedProcessPrefix)) {
            auto strippedKey = key;
            strippedKey.remove(CustomData::OptionPrefix + CustomData::BrowserAllowedProcessPrefix);

            const auto value = db->metadata()->customData()->value(key);
            if (strippedKey == clientProcess.hash && value == clientProcess.path) {
                return true;
            }
        }
    }

    return false;
}

ClientProcess BrowserClientRestrictions::getProcessPathAndHash(QLocalSocket* socket)
{
    if (!socket) {
        return {};
    }

    ClientProcess clientProcess;
    int socketDesc = socket->socketDescriptor();

#ifdef Q_OS_MACOS
    pid_t pid;
    socklen_t pidSize = sizeof(pid);
    auto ret = getsockopt(socketDesc, SOL_LOCAL, LOCAL_PEERPID, &pid, &pidSize);
    if (ret == -1) {
        return {};
    }

    char fullPath[PROC_PIDPATHINFO_MAXSIZE];
    ret = proc_pidpath(pid, fullPath, sizeof(fullPath));
    if (ret <= 0) {
        return {};
    }

    clientProcess.path = fullPath;
#elif Q_OS_WIN
    ULONG pid = 0;
    HANDLE socketHandle = reinterpret_cast<HANDLE>(socketDesc);
    auto res = GetNamedPipeClientProcessId(socketHandle, &pid);
    if (res == 0) {
        return {};
    }

    HANDLE processHandle = NULL;
    processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!processHandle) {
        return {};
    }

    TCHAR fullPath[MAX_PATH];
    if (GetModuleFileNameEx(processHandle, NULL, fullPath, MAX_PATH) == 0) {
        return {};
    }

    CloseHandle(processHandle);
    clientProcess.path = fullPath;
#elif Q_OS_LINUX
    pid_t pid;
    socklen_t pidSize = sizeof(pid);
    auto ret = getsockopt(socketDesc, SOL_SOCKET, SO_PEERCRED, &pid, &pidSize);
    if (ret == -1) {
        return {};
    }

    // Get process path from PID
    char buf[PATH_MAX];
    auto procPath = QString("/proc/%1/exe").arg(pid);
    auto fullPath = realpath(procPath.toStdString().c_str(), buf);
    if (!fullPath) {
        return {};
    }

    clientProcess.path = fullPath;
#elif Q_OS_UNIX

#endif
    if (clientProcess.path.isEmpty()) {
        return {};
    }

    clientProcess.hash = FileHash::getFileHash(fullPath, QCryptographicHash::Md5, 8192);
    return clientProcess;
}
