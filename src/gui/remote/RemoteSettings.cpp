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

#include "RemoteSettings.h"

#include "RemoteParams.h"

QString RemoteSettings::getName() const
{
    return m_name;
}
QString RemoteSettings::getDownloadCommand() const
{
    return m_downloadCommand;
}
QString RemoteSettings::getDownloadCommandInput() const
{
    return m_downloadCommandInput;
}
QString RemoteSettings::getUploadCommand() const
{
    return m_uploadCommand;
}
QString RemoteSettings::getUploadCommandInput() const
{
    return m_uploadCommandInput;
}
void RemoteSettings::setName(const QString& name)
{
    m_name = name;
}
void RemoteSettings::setDownloadCommand(const QString& downloadCommand)
{
    m_downloadCommand = downloadCommand;
}
void RemoteSettings::setDownloadCommandInput(const QString& downloadCommandInput)
{
    m_downloadCommandInput = downloadCommandInput;
}
void RemoteSettings::setUploadCommand(const QString& uploadCommand)
{
    m_uploadCommand = uploadCommand;
}
void RemoteSettings::setUploadCommandInput(const QString& uploadCommandInput)
{
    m_uploadCommandInput = uploadCommandInput;
}

QJsonObject RemoteSettings::toConfig() const
{
    QJsonObject config;
    config["name"] = m_name;
    config["downloadCommand"] = m_downloadCommand;
    config["downloadCommandInput"] = m_downloadCommandInput;
    config["uploadCommand"] = m_uploadCommand;
    config["uploadCommandInput"] = m_uploadCommandInput;
    return config;
}

void RemoteSettings::fromConfig(const QJsonObject& config)
{
    setName(config["name"].toString());
    setDownloadCommand(config["downloadCommand"].toString());
    setDownloadCommandInput(config["downloadCommandInput"].toString());
    setUploadCommand(config["uploadCommand"].toString());
    setUploadCommandInput(config["uploadCommandInput"].toString());
}

RemoteParams* RemoteSettings::toRemoteProgramParams() const
{
    auto* remoteProgramParams = new RemoteParams();
    remoteProgramParams->setCommandForDownload(m_downloadCommand);
    remoteProgramParams->setInputForDownload(m_downloadCommandInput);
    remoteProgramParams->setCommandForUpload(m_uploadCommand);
    remoteProgramParams->setInputForUpload(m_uploadCommandInput);
    return remoteProgramParams;
}
