/*
 *  Copyright (C) 2026 Thongvan Alexis <thongvan.alexis@proton.me>
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

#include "MockDropboxSyncProvider.h"

#include "remotesync/RemoteSyncParams.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>

QString MockDropboxSyncProvider::s_downloadSourcePath;
QString MockDropboxSyncProvider::s_nextDownloadFailureMessage;
RemoteHandler::ErrorKind MockDropboxSyncProvider::s_nextDownloadFailureKind = RemoteHandler::ErrorKind::Other;
int MockDropboxSyncProvider::s_downloadCallCount = 0;
int MockDropboxSyncProvider::s_uploadCallCount = 0;
int MockDropboxSyncProvider::s_refreshAuthCallCount = 0;
int MockDropboxSyncProvider::s_revokeTokenCallCount = 0;
bool MockDropboxSyncProvider::s_isAuthorizedOverride = true;

MockDropboxSyncProvider::MockDropboxSyncProvider(QObject* parent)
    : DropboxSyncProvider(parent)
{
}

void MockDropboxSyncProvider::setDownloadSourcePath(const QString& path)
{
    s_downloadSourcePath = path;
}

void MockDropboxSyncProvider::setNextDownloadFailure(const QString& errorMessage, RemoteHandler::ErrorKind kind)
{
    s_nextDownloadFailureMessage = errorMessage;
    s_nextDownloadFailureKind = kind;
}

int MockDropboxSyncProvider::downloadCallCount()
{
    return s_downloadCallCount;
}

int MockDropboxSyncProvider::uploadCallCount()
{
    return s_uploadCallCount;
}

int MockDropboxSyncProvider::refreshAuthCallCount()
{
    return s_refreshAuthCallCount;
}

int MockDropboxSyncProvider::revokeTokenCallCount()
{
    return s_revokeTokenCallCount;
}

void MockDropboxSyncProvider::resetCallCounts()
{
    s_downloadCallCount = 0;
    s_uploadCallCount = 0;
    s_refreshAuthCallCount = 0;
    s_revokeTokenCallCount = 0;
}

void MockDropboxSyncProvider::setIsAuthorizedOverride(bool authorized)
{
    s_isAuthorizedOverride = authorized;
}

bool MockDropboxSyncProvider::isAuthorized(const QJsonObject& config) const
{
    if (!s_isAuthorizedOverride) {
        return false;
    }
    return DropboxSyncProvider::isAuthorized(config);
}

RemoteHandler::RemoteResult MockDropboxSyncProvider::refreshAuth(const RemoteSyncParams* /*params*/)
{
    ++s_refreshAuthCallCount;
    // Empty stdOutput: provider declares "access token still valid, no rotation".
    // SyncEngine::doAuthenticate skips applyRefreshedTokens in this case.
    return {.success = true};
}

RemoteHandler::RemoteResult MockDropboxSyncProvider::download(const RemoteSyncParams* /*params*/)
{
    ++s_downloadCallCount;

    // One-shot failure injection: tests use this to drive the Test Connection
    // / sync failure paths without rebuilding the factory override.
    if (!s_nextDownloadFailureMessage.isEmpty()) {
        QString msg = s_nextDownloadFailureMessage;
        auto kind = s_nextDownloadFailureKind;
        s_nextDownloadFailureMessage.clear();
        s_nextDownloadFailureKind = RemoteHandler::ErrorKind::Other;
        return {.success = false, .errorMessage = msg, .kind = kind};
    }

    // One-shot consumption of the source path. The page-side Test Connection
    // sets the source once and expects one canonical-file copy; ANY later
    // download() (e.g. SyncEngine's sync-on-save / sync-on-open) must NOT
    // re-copy the same canonical file. Otherwise SyncEngine.doMerge runs
    // against the canonical kdbx every iteration -- since the canonical
    // kdbx has different timestamps than the live local db, Merger records
    // trivial history-item changes which leave m_modified=true, doSave
    // re-fires databaseSaved, and the queued onDatabaseSavedTriggerSync
    // (Qt::QueuedConnection at DatabaseWidget.cpp:1578, runs AFTER
    // m_syncInProgress is cleared) loops into the next sync. By consuming
    // the source, every chained sync sees filePath="" -> SyncEngine takes
    // the first-sync branch (SyncEngine.cpp:154) -> doSave on a now-clean
    // db -> markAsClean sees m_modified=false -> no databaseSaved emit ->
    // loop terminates after one iteration.
    const QString sourcePath = s_downloadSourcePath;
    s_downloadSourcePath.clear();

    // No canned source -> "remote file does not exist yet" first-sync mode.
    // SyncEngine treats {success=true, filePath=""} as the file-not-found
    // signal and skips merge, going straight to local save + upload.
    if (sourcePath.isEmpty() || !QFileInfo::exists(sourcePath)) {
        return {.success = true};
    }

    // Stream source -> brand-new temp path, NO QTemporaryFile + QFile::copy
    // dance. The earlier dance (open QTemporaryFile, close it, QFile::remove
    // the placeholder, QFile::copy on top) hit a Windows-specific failure:
    // QTemporaryFile retains an internal lock on its path even after close()
    // -- the subsequent remove silently no-ops and QFile::copy refuses to
    // overwrite. Building a unique path with QUuid and writing src bytes
    // directly into a freshly-created QFile dodges every Windows file-share
    // corner. Caller (page or SyncEngine) is still responsible for
    // QFile::remove of the returned path.
    const QString outPath = QDir::tempPath() + QStringLiteral("/keepassxc_mock_dropbox_")
                            + QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".kdbx");

    QFile src(sourcePath);
    if (!src.open(QIODevice::ReadOnly)) {
        return {.success = false,
                .errorMessage = QStringLiteral("MockDropboxSyncProvider: failed to open source %1: %2")
                                    .arg(sourcePath, src.errorString())};
    }
    const QByteArray data = src.readAll();
    src.close();

    QFile out(outPath);
    if (!out.open(QIODevice::WriteOnly)) {
        return {
            .success = false,
            .errorMessage =
                QStringLiteral("MockDropboxSyncProvider: failed to open dest %1: %2").arg(outPath, out.errorString())};
    }
    if (out.write(data) != data.size()) {
        out.close();
        QFile::remove(outPath);
        return {
            .success = false,
            .errorMessage =
                QStringLiteral("MockDropboxSyncProvider: failed to write dest %1: %2").arg(outPath, out.errorString())};
    }
    out.close();

    return {.success = true, .filePath = outPath};
}

RemoteHandler::RemoteResult MockDropboxSyncProvider::upload(const QString& /*filePath*/,
                                                            const RemoteSyncParams* /*params*/)
{
    ++s_uploadCallCount;
    return {.success = true};
}

RemoteHandler::RemoteResult MockDropboxSyncProvider::revokeToken(const DropboxSyncParams* /*params*/)
{
    ++s_revokeTokenCallCount;
    return {.success = true};
}
