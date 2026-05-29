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

#include "MockNextcloudSyncProvider.h"

#include "remotesync/RemoteSyncParams.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>

QString MockNextcloudSyncProvider::s_downloadSourcePath;
QString MockNextcloudSyncProvider::s_nextDownloadFailureMessage;
RemoteHandler::ErrorKind MockNextcloudSyncProvider::s_nextDownloadFailureKind = RemoteHandler::ErrorKind::Other;
int MockNextcloudSyncProvider::s_downloadCallCount = 0;
int MockNextcloudSyncProvider::s_uploadCallCount = 0;
int MockNextcloudSyncProvider::s_refreshAuthCallCount = 0;
int MockNextcloudSyncProvider::s_testConnectionCallCount = 0;
bool MockNextcloudSyncProvider::s_isAuthorizedOverride = true;

MockNextcloudSyncProvider::MockNextcloudSyncProvider(QObject* parent)
    : NextcloudSyncProvider(parent)
{
}

void MockNextcloudSyncProvider::setDownloadSourcePath(const QString& path)
{
    s_downloadSourcePath = path;
}

void MockNextcloudSyncProvider::setNextDownloadFailure(const QString& errorMessage, RemoteHandler::ErrorKind kind)
{
    s_nextDownloadFailureMessage = errorMessage;
    s_nextDownloadFailureKind = kind;
}

void MockNextcloudSyncProvider::setIsAuthorizedOverride(bool authorized)
{
    s_isAuthorizedOverride = authorized;
}

int MockNextcloudSyncProvider::downloadCallCount()
{
    return s_downloadCallCount;
}

int MockNextcloudSyncProvider::uploadCallCount()
{
    return s_uploadCallCount;
}

int MockNextcloudSyncProvider::refreshAuthCallCount()
{
    return s_refreshAuthCallCount;
}

int MockNextcloudSyncProvider::testConnectionCallCount()
{
    return s_testConnectionCallCount;
}

void MockNextcloudSyncProvider::resetCallCounts()
{
    s_downloadCallCount = 0;
    s_uploadCallCount = 0;
    s_refreshAuthCallCount = 0;
    s_testConnectionCallCount = 0;
}

bool MockNextcloudSyncProvider::isAuthorized(const QJsonObject& config) const
{
    if (!s_isAuthorizedOverride) {
        return false;
    }
    return NextcloudSyncProvider::isAuthorized(config);
}

RemoteHandler::RemoteResult MockNextcloudSyncProvider::refreshAuth(const RemoteSyncParams* /*params*/)
{
    ++s_refreshAuthCallCount;
    // Empty stdOutput: provider declares "auth still valid, no rotation".
    // SyncEngine::doAuthenticate skips applyRefreshedTokens in this case.
    return {.success = true};
}

RemoteHandler::RemoteResult MockNextcloudSyncProvider::testConnection(const NextcloudSyncParams* /*params*/)
{
    ++s_testConnectionCallCount;

    // Non-consuming peek (unlike download() below). Test Connection clicks are
    // safe to repeat against the same canned source. The page surface
    // distinguishes filePath="" ("Connected. File not found -- it will be
    // created on first sync.") from filePath != "" ("Nextcloud connection
    // successful."); we mirror that contract here so the user-visible banner
    // depends only on whether the test set a source.
    if (!s_downloadSourcePath.isEmpty() && QFileInfo::exists(s_downloadSourcePath)) {
        return {.success = true, .filePath = s_downloadSourcePath};
    }
    return {.success = true};
}

RemoteHandler::RemoteResult MockNextcloudSyncProvider::download(const RemoteSyncParams* /*params*/)
{
    ++s_downloadCallCount;

    // One-shot failure injection (mirrors MockDropboxSyncProvider).
    if (!s_nextDownloadFailureMessage.isEmpty()) {
        QString msg = s_nextDownloadFailureMessage;
        auto kind = s_nextDownloadFailureKind;
        s_nextDownloadFailureMessage.clear();
        s_nextDownloadFailureKind = RemoteHandler::ErrorKind::Other;
        return {.success = false, .errorMessage = msg, .kind = kind};
    }

    // One-shot consumption of the source path -- same chain-breaker rationale
    // as MockDropboxSyncProvider::download (the verbatim block-comment
    // explanation lives there; the contract is identical here).
    const QString sourcePath = s_downloadSourcePath;
    s_downloadSourcePath.clear();

    if (sourcePath.isEmpty() || !QFileInfo::exists(sourcePath)) {
        // First-sync mode: SyncEngine treats {success=true, filePath=""} as
        // file-not-found and skips merge, going straight to local save +
        // upload.
        return {.success = true};
    }

    // Stream source -> brand-new temp path (same Windows-share-friendly pattern
    // as MockDropboxSyncProvider::download).
    const QString outPath = QDir::tempPath() + QStringLiteral("/keepassxc_mock_nextcloud_")
                            + QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".kdbx");

    QFile src(sourcePath);
    if (!src.open(QIODevice::ReadOnly)) {
        return {.success = false,
                .errorMessage = QStringLiteral("MockNextcloudSyncProvider: failed to open source %1: %2")
                                    .arg(sourcePath, src.errorString())};
    }
    const QByteArray data = src.readAll();
    src.close();

    QFile out(outPath);
    if (!out.open(QIODevice::WriteOnly)) {
        return {.success = false,
                .errorMessage = QStringLiteral("MockNextcloudSyncProvider: failed to open dest %1: %2")
                                    .arg(outPath, out.errorString())};
    }
    if (out.write(data) != data.size()) {
        out.close();
        QFile::remove(outPath);
        return {.success = false,
                .errorMessage = QStringLiteral("MockNextcloudSyncProvider: failed to write dest %1: %2")
                                    .arg(outPath, out.errorString())};
    }
    out.close();

    return {.success = true, .filePath = outPath};
}

RemoteHandler::RemoteResult MockNextcloudSyncProvider::upload(const QString& /*filePath*/,
                                                              const RemoteSyncParams* /*params*/)
{
    ++s_uploadCallCount;
    return {.success = true};
}
