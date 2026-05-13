/*
 *  Copyright (C) 2026 Thongvan Alexis <thongvan.alexis@proton.me>
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

#ifndef KEEPASSXC_COMMANDSYNCPROVIDER_H
#define KEEPASSXC_COMMANDSYNCPROVIDER_H

#include "RemoteSyncProvider.h"

#include <QScopedPointer>

class RemoteHandler;

class CommandSyncProvider : public RemoteSyncProvider
{
    Q_OBJECT

public:
    explicit CommandSyncProvider(QObject* parent = nullptr);
    ~CommandSyncProvider() override = default;

    RemoteHandler::RemoteResult download(const RemoteSyncParams* params) override;
    RemoteHandler::RemoteResult upload(const QString& filePath, const RemoteSyncParams* params) override;
    RemoteHandler::RemoteResult refreshAuth(const RemoteSyncParams* params) override;
    void abort() override;

    QString displayName() const override;
    RemoteSyncParams* createParams() const override;

private:
    RemoteParams* toRemoteParams(const RemoteSyncParams* params) const;

    QScopedPointer<RemoteHandler> m_handler;
    Q_DISABLE_COPY(CommandSyncProvider)
};

#endif // KEEPASSXC_COMMANDSYNCPROVIDER_H
