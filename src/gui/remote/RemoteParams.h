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

#include <QMetaType>

class RemoteParams final
{
public:
    explicit RemoteParams() = default;
    ~RemoteParams() = default;

    bool hasDownloadCommand() const;
    bool hasUploadCommand() const;

    QString getCommandForDownload(const QString& destination);
    QString getInputForDownload(const QString& destination);
    QString getCommandForUpload(const QString& source);
    QString getInputForUpload(const QString& source);

    void setCommandForDownload(const QString& downloadCommand);
    void setInputForDownload(const QString& downloadCommandInput);
    void setCommandForUpload(const QString& uploadCommand);
    void setInputForUpload(const QString& uploadCommandInput);

private:
    QString resolveCommandOrInput(const QString& input, const QString& tempDatabasePath);

    QString m_downloadCommand;
    QString m_downloadCommandInput;
    QString m_uploadCommand;
    QString m_uploadCommandInput;
};

Q_DECLARE_METATYPE(RemoteParams);

#endif // KEEPASSXC_REMOTEPARAMS_H
