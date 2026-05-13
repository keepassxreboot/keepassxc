/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_REMOTEHANDLER_H
#define KEEPASSXC_REMOTEHANDLER_H

#include <QObject>

class Database;
class RemoteProcess;
struct RemoteParams;

class RemoteHandler : public QObject
{
    Q_OBJECT

public:
    explicit RemoteHandler(QObject* parent = nullptr);
    ~RemoteHandler() override = default;

    // Provider-emitted error classification. Carrying the kind on the result
    // object lets retry/dispatch logic decide based on a machine-readable
    // signal instead of substring-matching tr()'d user-facing strings, which
    // silently breaks under translation.
    enum class ErrorKind
    {
        Other,
        AuthExpired,
        AuthRevoked,
        Network,
        RateLimit, // 429 Too Many Requests; transient 423 Locked.
        Conflict, // Concurrent modification (412 Precondition Failed / rev mismatch).
        NotFound,
        Quota, // 507 Insufficient Storage.
        ServerError, // Generic 5xx (provider does not distinguish further).
        Permission, // 403 when not auth-revoked (e.g. read-only share).
        Aborted // User-initiated cancel (not a server error).
    };

    struct RemoteResult
    {
        bool success;
        QString errorMessage;
        QString filePath;
        QString stdOutput;
        QString stdError;
        // Provider-set classification. errorMessage is for the user
        // (localized); this field is for control flow.
        ErrorKind kind = ErrorKind::Other;
    };

    RemoteResult download(const RemoteParams* params);
    RemoteResult upload(const QString& filePath, const RemoteParams* params);

    // Used for testing only
    static void setRemoteProcessFunc(std::function<QScopedPointer<RemoteProcess>(QObject*)> func);

private:
    static std::function<QScopedPointer<RemoteProcess>(QObject*)> m_createRemoteProcess;
    static QString m_tempFileLocation;

    Q_DISABLE_COPY(RemoteHandler)
};

#endif // KEEPASSXC_REMOTEHANDLER_H
