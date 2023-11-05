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
#include <QMap>
#include <QObject>
#include <QVariant>

#include "RemoteParams.h"

class RemoteSettings : QObject
{
    Q_OBJECT

public:
    explicit RemoteSettings(QObject* parent = nullptr);
    ~RemoteSettings() override = default;

    QString getName();
    QString getDownloadCommand();
    QString getDownloadCommandInput();
    QString getUploadCommand();
    QString getUploadCommandInput();

    void setName(QString name);
    void setDownloadCommand(QString downloadCommand);
    void setDownloadCommandInput(QString downloadCommandInput);
    void setUploadCommand(QString uploadCommand);
    void setUploadCommandInput(QString uploadCommandInput);

    QJsonObject toConfig();
    void fromConfig(const QJsonObject&);

    RemoteParams* toRemoteProgramParams();

private:
    QString m_name;
    QString m_downloadCommand;
    QString m_downloadCommandInput;
    QString m_uploadCommand;
    QString m_uploadCommandInput;
};

#endif // KEEPASSXC_REMOTESETTINGS_H
