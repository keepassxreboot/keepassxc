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

#ifndef KEEPASSXC_REMOTESETTINGS_H
#define KEEPASSXC_REMOTESETTINGS_H

#include <QJsonObject>

class RemoteParams;

class RemoteSettings final
{
public:
    explicit RemoteSettings() = default;
    ~RemoteSettings() = default;

    QString getName() const;
    QString getDownloadCommand() const;
    QString getDownloadCommandInput() const;
    QString getUploadCommand() const;
    QString getUploadCommandInput() const;

    void setName(const QString& name);
    void setDownloadCommand(const QString& downloadCommand);
    void setDownloadCommandInput(const QString& downloadCommandInput);
    void setUploadCommand(const QString& uploadCommand);
    void setUploadCommandInput(const QString& uploadCommandInput);

    void fromConfig(const QJsonObject&);
    QJsonObject toConfig() const;

    RemoteParams* toRemoteProgramParams() const;

private:
    QString m_name;
    QString m_downloadCommand;
    QString m_downloadCommandInput;
    QString m_uploadCommand;
    QString m_uploadCommandInput;
};

#endif // KEEPASSXC_REMOTESETTINGS_H
