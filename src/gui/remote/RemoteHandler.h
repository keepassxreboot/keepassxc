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

    struct RemoteResult
    {
        bool success;
        QString errorMessage;
        QString filePath;
        QString stdOutput;
        QString stdError;
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
