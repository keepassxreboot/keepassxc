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

bool RemoteParams::hasDownloadCommand() const
{
    return m_downloadCommand.length() > 0;
}

bool RemoteParams::hasUploadCommand() const
{
    return m_uploadCommand.length() > 0;
}

void RemoteParams::setCommandForDownload(const QString& downloadCommand)
{
    m_downloadCommand = downloadCommand;
}

void RemoteParams::setInputForDownload(const QString& downloadCommandInput)
{
    m_downloadCommandInput = downloadCommandInput;
}

void RemoteParams::setCommandForUpload(const QString& uploadCommand)
{
    m_uploadCommand = uploadCommand;
}
void RemoteParams::setInputForUpload(const QString& uploadCommandInput)
{
    m_uploadCommandInput = uploadCommandInput;
}

QString RemoteParams::getCommandForDownload(const QString& destination)
{
    return resolveCommandOrInput(m_downloadCommand, destination);
}

QString RemoteParams::getInputForDownload(const QString& destination)
{
    return resolveCommandOrInput(m_downloadCommandInput, destination);
}

QString RemoteParams::getCommandForUpload(const QString& source)
{
    return resolveCommandOrInput(m_uploadCommand, source);
}

QString RemoteParams::getInputForUpload(const QString& source)
{
    return resolveCommandOrInput(m_uploadCommandInput, source);
}

QString RemoteParams::resolveCommandOrInput(const QString& input, const QString& tempDatabasePath)
{
    QString resolved = input;
    return resolved.replace("{TEMP_DATABASE}", tempDatabasePath);
}
