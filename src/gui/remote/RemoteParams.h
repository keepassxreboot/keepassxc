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

#ifndef KEEPASSXC_REMOTEPARAMS_H
#define KEEPASSXC_REMOTEPARAMS_H

#include "RemoteParams.h"
#include <QMetaType>
#include <QStringList>

class RemoteParams
{
public:
    RemoteParams() = default;
    ~RemoteParams() = default;

    QString getCommandForDownload(QString destination);
    QString getInputForDownload(QString destination);
    QString getCommandForUpload(QString source);
    QString getInputForUpload(QString source);

    void setCommandForDownload(QString downloadCommand);
    void setInputForDownload(QString downloadCommandInput);
    void setCommandForUpload(QString uploadCommand);
    void setInputForUpload(QString uploadCommandInput);

private:
    QString resolveCommandOrInput(QString input, const QString& tempDatabasePath);

    QString m_downloadCommand;
    QString m_downloadCommandInput;
    QString m_uploadCommand;
    QString m_uploadCommandInput;
};

Q_DECLARE_METATYPE(RemoteParams);

#endif // KEEPASSXC_REMOTEPARAMS_H
