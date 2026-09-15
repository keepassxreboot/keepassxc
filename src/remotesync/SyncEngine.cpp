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

#include "SyncEngine.h"

#include <QFile>

#include "core/Database.h"
#include "core/Merger.h"
#include "remotesync/RemoteSyncParams.h"
#include "remotesync/RemoteSyncProvider.h"

SyncEngine::SyncEngine(QSharedPointer<Database> db, SaveFn saveFn, QObject* parent)
    : QObject(parent)
    , m_db(std::move(db))
    , m_saveFn(std::move(saveFn))
{
    Q_ASSERT(m_saveFn);
}

SyncEngine::~SyncEngine()
{
    // Safety net: clean up any remaining temp file
    if (!m_downloadedFilePath.isEmpty()) {
        QFile::remove(m_downloadedFilePath);
        m_downloadedFilePath.clear();
    }
}

SyncEngine::State SyncEngine::state() const
{
    return m_state;
}

QString SyncEngine::downloadedFilePath() const
{
    return m_downloadedFilePath;
}

RemoteHandler::ErrorKind SyncEngine::lastErrorKind() const
{
    return m_lastErrorKind;
}

bool SyncEngine::startSync(RemoteSyncProvider* provider, RemoteSyncParams* params)
{
    if (m_state != State::Idle) {
        emit syncError(tr("Sync already in progress."));
        return false;
    }

    m_provider = provider;
    m_params = params;
    m_cancelRequested = false;
    m_downloadedFilePath.clear();
    m_changeList.clear();
    m_lastErrorKind = RemoteHandler::ErrorKind::Other;

    doAuthenticate();
    return true;
}

void SyncEngine::cancel()
{
    if (m_state == State::Idle) {
        return;
    }
    m_cancelRequested = true;
}

void SyncEngine::setState(State newState)
{
    if (m_state == newState) {
        return;
    }
    m_state = newState;
    emit stateChanged(m_state);
}

void SyncEngine::doAuthenticate()
{
    setState(State::Authenticating);
    emit syncProgress(10, tr("Refreshing authentication..."));

    auto result = m_provider->refreshAuth(m_params);
    if (!result.success) {
        qWarning("[SyncEngine] refreshAuth FAILED: %s", qPrintable(result.errorMessage));
        m_lastErrorKind = result.kind;
        handleError(result.errorMessage);
        return;
    }

    // If refreshAuth returned updated token data, apply it to in-memory
    // params. A parse/apply failure surfaces as an authentication error.
    if (!result.stdOutput.isEmpty()) {
        if (!m_provider->applyRefreshedTokens(result.stdOutput, m_params)) {
            handleError(tr("Authentication expired. Re-authorize in Database > Settings > Cloud Sync."));
            return;
        }
        emit refreshedTokenData(result.stdOutput);
    }

    if (m_cancelRequested) {
        cleanup();
        emit syncFinished(false, tr("Sync cancelled."));
        return;
    }

    doDownload();
}

void SyncEngine::doDownload()
{
    setState(State::Downloading);
    emit syncProgress(25, tr("Downloading remote database..."));

    auto result = m_provider->download(m_params);
    if (!result.success) {
        qWarning("[SyncEngine] download FAILED: %s", qPrintable(result.errorMessage));
        m_lastErrorKind = result.kind;
        handleError(result.errorMessage);
        return;
    }

    m_downloadedFilePath = result.filePath;

    if (m_cancelRequested) {
        cleanup();
        emit syncFinished(false, tr("Sync cancelled."));
        return;
    }

    // First-sync convention: providers return {success=true, filePath=""}
    // when the remote file doesn't exist yet (e.g. Dropbox 404
    // path/not_found, Nextcloud first-sync). Skip merge in that case --
    // there's nothing to merge against -- and go straight to the local
    // save + upload that creates the file on the remote side.
    if (m_downloadedFilePath.isEmpty()) {
        doSave();
        return;
    }

    doMerge();
}

void SyncEngine::doMerge()
{
    setState(State::Merging);
    emit syncProgress(50, tr("Merging databases..."));

    QSharedPointer<Database> remoteDb = QSharedPointer<Database>::create();
    QString error;
    bool opened = remoteDb->open(m_downloadedFilePath, m_db->key(), &error);
    if (!opened) {
        // Fallback: if the user just changed the master key, the remote
        // still holds the old one. Database::syncPreviousKey() returns the
        // snapshot captured at change-key time; retry with that. doUpload
        // clears the snapshot on success so the remote is migrated.
        auto previousKey = m_db->syncPreviousKey();
        if (previousKey) {
            opened = remoteDb->open(m_downloadedFilePath, previousKey, &error);
        }
    }
    if (!opened) {
        // Remote DB needs a different key. Hand off the temp file to the
        // receiver -- DatabaseWidget keeps it alive across the unlock
        // dialog (which needs to read it back) and removes it when the
        // dialog completes. Clearing m_downloadedFilePath here also stops
        // the engine destructor from racing the receiver to delete it.
        QString filePath = m_downloadedFilePath;
        m_downloadedFilePath.clear();
        m_cancelRequested = false;
        m_provider = nullptr;
        m_params = nullptr;
        setState(State::Idle);
        emit remoteDbNeedsKey(filePath);
        return;
    }
    remoteDb->markAsTemporaryDatabase();

    // One-way merge: remote INTO local
    Merger merger(remoteDb.data(), m_db.data());
    m_changeList = merger.merge();

    if (m_cancelRequested) {
        // Rollback: re-open from disk to restore pre-merge in-memory state
        m_db->open(m_db->filePath(), m_db->key());
        cleanup();
        emit syncFinished(false, tr("Sync cancelled."));
        return;
    }

    doSave();
}

void SyncEngine::doSave()
{
    setState(State::Saving);
    emit syncProgress(65, tr("Saving local database..."));

    QString error;
    if (!m_saveFn(error)) {
        qWarning("[SyncEngine] save FAILED: %s", qPrintable(error));
        handleError(tr("Failed to save local database: %1").arg(error));
        return;
    }

    if (m_cancelRequested) {
        cleanup();
        emit syncFinished(false, tr("Sync cancelled."));
        return;
    }

    doUpload();
}

void SyncEngine::doUpload()
{
    setState(State::Uploading);
    emit syncProgress(85, tr("Uploading merged database..."));

    // Upload the LOCAL .kdbx file (not the downloaded temp file)
    auto result = m_provider->upload(m_db->filePath(), m_params);
    cleanup(); // Always cleanup temp files

    if (!result.success) {
        qWarning("[SyncEngine] upload FAILED: %s", qPrintable(result.errorMessage));
        m_lastErrorKind = result.kind;
        // Upload failed, but the local save already succeeded -- no rollback.
        emit syncFinished(false, tr("Upload failed: %1").arg(result.errorMessage));
        return;
    }

    // Remote now holds the same master key as the local DB; the change-key
    // snapshot (if any) is no longer needed.
    m_db->clearSyncPreviousKey();

    emit syncProgress(100, tr("Sync complete."));
    emit syncFinished(true, QString());
}

void SyncEngine::handleError(const QString& errorMessage)
{
    cleanup();
    emit syncFinished(false, errorMessage);
}

void SyncEngine::cleanup()
{
    // Remove downloaded temp file if it exists
    if (!m_downloadedFilePath.isEmpty()) {
        QFile::remove(m_downloadedFilePath);
        m_downloadedFilePath.clear();
    }

    m_cancelRequested = false;
    m_provider = nullptr;
    m_params = nullptr;
    setState(State::Idle);
}
