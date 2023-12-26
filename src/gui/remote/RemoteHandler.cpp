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

#include "gui/remote/RemoteParams.h"
#include "gui/remote/RemoteProcess.h"

#include "core/AsyncTask.h"
#include "core/Database.h"

namespace
{
    QString getTempFileLocation()
    {
        QString uuid = QUuid::createUuid().toString().remove(0, 1);
        uuid.chop(1);
        return QDir::toNativeSeparators(QDir::temp().absoluteFilePath("RemoteDatabase-" + uuid + ".kdbx"));
    }
} // namespace

std::function<QScopedPointer<RemoteProcess>(QObject*)> RemoteHandler::m_createRemoteProcess([](QObject* parent) {
    return QScopedPointer<RemoteProcess>(new RemoteProcess(parent));
});

RemoteHandler::RemoteHandler(QObject* parent)
    : QObject(parent)
{
}

void RemoteHandler::download(RemoteParams* params)
{
    AsyncTask::runThenCallback(
        [&, params] { return downloadInternal(params); }, this, [&](auto result) { emit downloadFinished(result); });
}

void RemoteHandler::upload(const QSharedPointer<Database>& db, RemoteParams* params)
{
    AsyncTask::runThenCallback([&, db, params] { return uploadInternal(db, params); },
                               this,
                               [&](auto result) { emit uploadFinished(result); });
}

RemoteHandler::RemoteResult RemoteHandler::downloadInternal(RemoteParams* params)
{
    RemoteResult result;

    auto remoteProcess = m_createRemoteProcess(this);
    QString destination = getTempFileLocation();
    auto downloadCommand = params->getCommandForDownload(destination);
    remoteProcess->start(downloadCommand);
    auto input = params->getInputForDownload(destination);
    if (!input.isEmpty()) {
        remoteProcess->write(input + "\n");
        remoteProcess->waitForBytesWritten();
        remoteProcess->closeWriteChannel();
    }

    bool finished = remoteProcess->waitForFinished(10000);
    int statusCode = remoteProcess->exitCode();

    // TODO: For future use
    result.stdOutput = remoteProcess->readOutput();
    result.stdError = remoteProcess->readError();

    if (finished && statusCode == 0) {
        result.success = true;
        result.filePath = destination;
    } else if (finished) {
        result.success = false;
        result.errorMessage = tr("Command `%1` exited with status code: %3").arg(downloadCommand).arg(statusCode);
    } else {
        remoteProcess->kill();
        result.success = false;
        result.errorMessage = tr("Command `%1` did not finish in time. Process was killed.").arg(downloadCommand);
    }

    return result;
}

RemoteHandler::RemoteResult RemoteHandler::uploadInternal(const QSharedPointer<Database>& db, RemoteParams* params)
{
    RemoteResult result;

    auto remoteProcess = m_createRemoteProcess(this);
    QString source = db->filePath();
    auto uploadCommand = params->getCommandForUpload(source);
    remoteProcess->start(uploadCommand);
    auto input = params->getInputForUpload(source);
    if (!input.isEmpty()) {
        remoteProcess->write(input + "\n");
        remoteProcess->waitForBytesWritten();
        remoteProcess->closeWriteChannel();
    }

    bool finished = remoteProcess->waitForFinished(10000);
    int statusCode = remoteProcess->exitCode();

    // TODO: For future use
    result.stdOutput = remoteProcess->readOutput();
    result.stdError = remoteProcess->readError();

    if (finished && statusCode == 0) {
        result.success = true;
    } else if (finished) {
        result.success = false;
        result.errorMessage = tr("Failed to upload merged database. Command `%1` exited with status code: %3")
                                  .arg(uploadCommand)
                                  .arg(statusCode);
    } else {
        remoteProcess->kill();
        result.success = false;
        result.errorMessage =
            tr("Failed to upload merged database. Command `%1` did not finish in time. Process was killed.")
                .arg(uploadCommand)
                .arg(statusCode);
    }

    return result;
}
