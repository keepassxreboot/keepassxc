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

#ifndef KEEPASSXC_SYNCENGINE_H
#define KEEPASSXC_SYNCENGINE_H

#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <functional>

#include "core/Merger.h"
#include "gui/remote/RemoteHandler.h" // for ErrorKind

class Database;
class RemoteSyncProvider;
struct RemoteSyncParams;

class SyncEngine : public QObject
{
    Q_OBJECT

public:
    enum class State
    {
        Idle,
        Authenticating,
        Downloading,
        Merging,
        Saving,
        Uploading
    };
    Q_ENUM(State)

    // Callback that performs the local save and returns true on success,
    // writing any failure message into the out-parameter. Supplied by the
    // caller (DatabaseWidget) so the merged DB goes through the same save
    // policy as a normal user-initiated save -- cloud sync MUST NOT bypass
    // save policy.
    using SaveFn = std::function<bool(QString& errorMessage)>;

    explicit SyncEngine(QSharedPointer<Database> db, SaveFn saveFn, QObject* parent = nullptr);
    ~SyncEngine() override;

    State state() const;

    // The ErrorKind from the last failed sync step (download / merge / save /
    // upload / refreshAuth). Cleared to Other at the start of each startSync.
    // Cached here so consumers (DatabaseWidget, MainWindow) classify on a
    // machine-readable signal instead of substring-matching tr()'d error
    // strings.
    RemoteHandler::ErrorKind lastErrorKind() const;

    // Start a sync operation using the given provider and params.
    // The provider must outlive the sync call. SyncEngine does NOT take ownership.
    // Returns false if a sync is already in progress.
    bool startSync(RemoteSyncProvider* provider, RemoteSyncParams* params);

    // Request cancellation. Cancel is checked between steps (not mid-operation).
    void cancel();

    // Accessor for test verification of temp file cleanup
    QString downloadedFilePath() const;

signals:
    void stateChanged(SyncEngine::State newState);
    void syncProgress(int percentage, const QString& message);
    void syncFinished(bool success, const QString& message);
    void syncError(const QString& errorMessage);
    void remoteDbNeedsKey(const QString& filePath);
    // Emitted when refreshAuth() returns updated token data (JSON in stdOutput).
    // Caller should parse and persist to CustomData.
    void refreshedTokenData(const QString& tokenDataJson);

private:
    void setState(State newState);
    void doAuthenticate();
    void doDownload();
    void doMerge();
    void doSave();
    void doUpload();
    void handleError(const QString& errorMessage);
    void cleanup();

    QSharedPointer<Database> m_db;
    SaveFn m_saveFn;
    RemoteSyncProvider* m_provider = nullptr; // non-owning
    RemoteSyncParams* m_params = nullptr;
    State m_state = State::Idle;
    bool m_cancelRequested = false;
    QString m_downloadedFilePath;
    Merger::ChangeList m_changeList;
    RemoteHandler::ErrorKind m_lastErrorKind = RemoteHandler::ErrorKind::Other;

    Q_DISABLE_COPY(SyncEngine)
};

#endif // KEEPASSXC_SYNCENGINE_H
