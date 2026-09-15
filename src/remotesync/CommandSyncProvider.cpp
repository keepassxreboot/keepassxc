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

#include "CommandSyncProvider.h"

#include "RemoteSyncParams.h"
#include "gui/remote/RemoteHandler.h"
#include "gui/remote/RemoteSettings.h"

CommandSyncProvider::CommandSyncProvider(QObject* parent)
    : RemoteSyncProvider(parent)
    , m_handler(new RemoteHandler(this))
{
}

RemoteParams* CommandSyncProvider::toRemoteParams(const RemoteSyncParams* params) const
{
    // Safe: the factory and buildParamsFromConfig pair providers and params
    // by type, so a CommandSyncProvider only ever receives CommandSyncParams.
    return new RemoteParams(static_cast<const CommandSyncParams*>(params)->toRemoteParams());
}

RemoteHandler::RemoteResult CommandSyncProvider::download(const RemoteSyncParams* params)
{
    QScopedPointer<RemoteParams> remoteParams(toRemoteParams(params));
    return m_handler->download(remoteParams.data());
}

RemoteHandler::RemoteResult CommandSyncProvider::upload(const QString& filePath, const RemoteSyncParams* params)
{
    QScopedPointer<RemoteParams> remoteParams(toRemoteParams(params));
    return m_handler->upload(filePath, remoteParams.data());
}

RemoteHandler::RemoteResult CommandSyncProvider::refreshAuth(const RemoteSyncParams* params)
{
    Q_UNUSED(params)
    // Command-based providers do not have auth refresh -- return success as no-op
    return {.success = true};
}

void CommandSyncProvider::abort()
{
    // NOTE: RemoteHandler does not expose an external abort mechanism.
    // CommandSyncProvider sync operations cancel on the next polling boundary,
    // not immediately. This is a CommandSyncProvider-specific limitation;
    // network-based providers (Dropbox, Nextcloud) have proper abort via
    // their QNetworkAccessManager.
}

QString CommandSyncProvider::displayName() const
{
    return QStringLiteral("Command");
}

RemoteSyncParams* CommandSyncProvider::createParams() const
{
    return new CommandSyncParams();
}
