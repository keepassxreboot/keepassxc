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

#ifndef KEEPASSXC_MOCKDROPBOXSYNCPROVIDER_H
#define KEEPASSXC_MOCKDROPBOXSYNCPROVIDER_H

#include "remotesync/DropboxSyncProvider.h"

// Test double for DropboxSyncProvider: replaces all four network-fronted
// operations (download / upload / refreshAuth / revokeToken) with canned
// successes. Drop-in via RemoteSyncProvider::setFactoryOverrideForTest --
// callers must NOT call setNetworkAccessManager on the mock (the overrides
// never touch QNAM).
//
// Default behavior is "happy path for an already-authorized user":
//   * refreshAuth -> success, empty stdOutput (no token rotation)
//   * download    -> success; filePath copied from s_downloadSourcePath if set,
//                    empty otherwise (= "remote file does not exist yet",
//                    SyncEngine takes the first-sync branch and skips merge)
//   * upload      -> success
//   * revokeToken -> success
//
// Per-test customization is via the static setters below. Because the page
// and DatabaseWidget each create() their own provider instance, instance-level
// configuration would not reach both -- statics are the legitimate exemption.
class MockDropboxSyncProvider : public DropboxSyncProvider
{
    Q_OBJECT

public:
    explicit MockDropboxSyncProvider(QObject* parent = nullptr);
    ~MockDropboxSyncProvider() override = default;

    RemoteHandler::RemoteResult download(const RemoteSyncParams* params) override;
    RemoteHandler::RemoteResult upload(const QString& filePath, const RemoteSyncParams* params) override;
    RemoteHandler::RemoteResult refreshAuth(const RemoteSyncParams* params) override;
    RemoteHandler::RemoteResult revokeToken(const DropboxSyncParams* params) override;

    // Set the source .kdbx file that the NEXT download() call will copy to
    // a temp path and return. One-shot: download() consumes the source and
    // clears it, so any subsequent download() falls back to "remote file
    // not found" (filePath=""). This breaks the chained-sync loop that
    // would otherwise result from re-merging a static canonical file every
    // iteration (the merger logs trivial history changes -> m_modified=true
    // -> save re-emits databaseSaved -> queued onDatabaseSavedTriggerSync
    // starts another sync). Passing an empty string explicitly resets to
    // first-sync mode without consuming a stored source.
    static void setDownloadSourcePath(const QString& path);

    // Set the next failure to return from download(). After being returned
    // once, the failure is cleared and subsequent calls revert to success.
    // Used to inject Test Connection / sync failure scenarios.
    static void setNextDownloadFailure(const QString& errorMessage,
                                       RemoteHandler::ErrorKind kind = RemoteHandler::ErrorKind::Other);

    // Test-only chain breaker for the post-sync re-trigger loop. When set
    // to false, isAuthorized() returns false unconditionally -- which makes
    // both DatabaseWidget::isCloudSyncAuthorized() (in
    // onDatabaseSavedTriggerSync) and syncWithCloud()'s own isAuthorized()
    // check (DatabaseWidget.cpp:1264) short-circuit, so no new sync starts.
    // Use this to assert "this user action did NOT trigger a sync" without
    // being polluted by the background save->databaseSaved->queued-slot->sync
    // chain (the chain runs because Database::save updates RandomSlug on
    // every save, which always re-emits databaseSaved -- mock-fast saves
    // never let the 150ms m_modifiedTimer fire to reload RemoteSettings).
    // Default true (matches production behavior).
    static void setIsAuthorizedOverride(bool authorized);

    bool isAuthorized(const QJsonObject& config) const override;

    // Call counters for assertions.
    static int downloadCallCount();
    static int uploadCallCount();
    static int refreshAuthCallCount();
    static int revokeTokenCallCount();
    static void resetCallCounts();

private:
    static QString s_downloadSourcePath;
    static QString s_nextDownloadFailureMessage;
    static RemoteHandler::ErrorKind s_nextDownloadFailureKind;
    static int s_downloadCallCount;
    static int s_uploadCallCount;
    static int s_refreshAuthCallCount;
    static int s_revokeTokenCallCount;
    static bool s_isAuthorizedOverride;
};

#endif // KEEPASSXC_MOCKDROPBOXSYNCPROVIDER_H
