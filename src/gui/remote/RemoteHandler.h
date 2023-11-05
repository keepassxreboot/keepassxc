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

#ifndef KEEPASSXC_REMOTEHANDLER_H
#define KEEPASSXC_REMOTEHANDLER_H

#include "core/Database.h"
#include "gui/remote/RemoteParams.h"
#include "gui/remote/RemoteProcess.h"

#include <QObject>

class RemoteHandler : public QObject
{
    Q_OBJECT

public:
    explicit RemoteHandler(QObject* parent = nullptr);
    ~RemoteHandler() override;
    void download(RemoteParams* remoteProgramParams);

signals:
    void downloadFromRemote(RemoteParams*);
    void uploadToRemote(const QSharedPointer<Database>&, RemoteParams*);

    void downloadedSuccessfullyTo(const QString& filePath);
    void downloadError(const QString& errorMessage);
    void uploadSuccess();
    void uploadError(const QString& errorMessage);

private slots:
    void upload(const QSharedPointer<Database>&, RemoteParams* remoteProgramParams);

private:
    void downloadInternal(RemoteParams* remoteProgramParams);
    void uploadInternal(const QSharedPointer<Database>& remoteSyncedDb, RemoteParams* remoteProgramParams);
};

#endif // KEEPASSXC_REMOTEHANDLER_H
