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

#include "RemoteHandler.h"
#include "RemoteProcessFactory.h"
#include "core/AsyncTask.h"

RemoteHandler::RemoteHandler(QObject* parent)
    : QObject(parent)
{
    connect(this, &RemoteHandler::downloadFromRemote, &RemoteHandler::download);
    connect(this, &RemoteHandler::uploadToRemote, &RemoteHandler::upload);
}

RemoteHandler::~RemoteHandler()
{
}

void RemoteHandler::download(RemoteParams* remoteProgramParams)
{
    AsyncTask::runAndWaitForFinished([&] { this->downloadInternal(remoteProgramParams); });
}

void RemoteHandler::downloadInternal(RemoteParams* remoteProgramParams)
{
    auto remoteProcess = RemoteProcessFactory::createRemoteProcess();
    QString destination = remoteProcess->getTempFileLocation();
    auto downloadCommand = remoteProgramParams->getCommandForDownload(destination);
    remoteProcess->start(downloadCommand);
    auto input = remoteProgramParams->getInputForDownload(destination);
    if (!input.isEmpty()) {
        remoteProcess->write(input + "\n");
        remoteProcess->waitForBytesWritten();
        remoteProcess->closeWriteChannel();
    }

    bool finished = remoteProcess->waitForFinished(10000);
    int statusCode = remoteProcess->exitCode();
    if (finished && statusCode == 0) {
        emit downloadedSuccessfullyTo(destination);
        return;
    }

    if (finished) {
        emit downloadError(tr("Command `%1` exited with status code: %3").arg(downloadCommand).arg(statusCode));
    } else {
        remoteProcess->kill();
        emit downloadError(tr("Command `%1` did not finish in time. Process was killed.").arg(downloadCommand));
    }
}

void RemoteHandler::upload(const QSharedPointer<Database>& remoteSyncedDb, RemoteParams* remoteProgramParams)
{
    AsyncTask::runAndWaitForFinished([&] { this->uploadInternal(remoteSyncedDb, remoteProgramParams); });
}

void RemoteHandler::uploadInternal(const QSharedPointer<Database>& remoteSyncedDb, RemoteParams* remoteProgramParams)
{
    auto remoteProcess = RemoteProcessFactory::createRemoteProcess();
    QString source = remoteSyncedDb->filePath();
    auto uploadCommand = remoteProgramParams->getCommandForUpload(source);
    remoteProcess->start(uploadCommand);
    auto input = remoteProgramParams->getInputForUpload(source);
    if (!input.isEmpty()) {
        remoteProcess->write(input + "\n");
        remoteProcess->waitForBytesWritten();
        remoteProcess->closeWriteChannel();
    }
    bool finished = remoteProcess->waitForFinished(10000);
    int statusCode = remoteProcess->exitCode();
    if (finished && statusCode == 0) {
        emit uploadSuccess();
        return;
    }

    if (finished) {
        emit uploadError(tr("Failed to upload merged database. Command `%1` exited with status code: %3")
                             .arg(uploadCommand)
                             .arg(statusCode));
    } else {
        remoteProcess->kill();
        emit uploadError(
            tr("Failed to upload merged database. Command `%1` did not finish in time. Process was killed.")
                .arg(uploadCommand)
                .arg(statusCode));
    }
}
