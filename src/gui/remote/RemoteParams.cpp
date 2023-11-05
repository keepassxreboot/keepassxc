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

#include "RemoteParams.h"

#include <utility>

void RemoteParams::setCommandForDownload(QString downloadCommand)
{
    m_downloadCommand = std::move(downloadCommand);
}

void RemoteParams::setInputForDownload(QString downloadCommandInput)
{
    m_downloadCommandInput = std::move(downloadCommandInput);
}

void RemoteParams::setCommandForUpload(QString uploadCommand)
{
    m_uploadCommand = std::move(uploadCommand);
}
void RemoteParams::setInputForUpload(QString uploadCommandInput)
{
    m_uploadCommandInput = std::move(uploadCommandInput);
}

QString RemoteParams::getCommandForDownload(QString destination)
{
    return resolveCommandOrInput(m_downloadCommand, destination);
}

QString RemoteParams::getInputForDownload(QString destination)
{
    return resolveCommandOrInput(m_downloadCommandInput, destination);
}

QString RemoteParams::getCommandForUpload(QString source)
{
    return resolveCommandOrInput(m_uploadCommand, source);
}

QString RemoteParams::getInputForUpload(QString source)
{
    return resolveCommandOrInput(m_uploadCommandInput, source);
}

QString RemoteParams::resolveCommandOrInput(QString input, const QString& tempDatabasePath)
{
    auto resolved = input.replace("{TEMP_DATABASE}", tempDatabasePath);
    return resolved;
}
