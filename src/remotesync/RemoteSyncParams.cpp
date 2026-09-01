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

#include "RemoteSyncParams.h"

#include "gui/remote/RemoteSettings.h"

CommandSyncParams::CommandSyncParams(const RemoteParams& params)
{
    type = QStringLiteral("command");
    name = params.name;
    downloadCommand = params.downloadCommand;
    downloadInput = params.downloadInput;
    downloadTimeoutMsec = params.downloadTimeoutMsec;
    uploadCommand = params.uploadCommand;
    uploadInput = params.uploadInput;
    uploadTimeoutMsec = params.uploadTimeoutMsec;
}

RemoteParams CommandSyncParams::toRemoteParams() const
{
    RemoteParams params;
    params.name = name;
    params.downloadCommand = downloadCommand;
    params.downloadInput = downloadInput;
    params.downloadTimeoutMsec = downloadTimeoutMsec;
    params.uploadCommand = uploadCommand;
    params.uploadInput = uploadInput;
    params.uploadTimeoutMsec = uploadTimeoutMsec;
    return params;
}
