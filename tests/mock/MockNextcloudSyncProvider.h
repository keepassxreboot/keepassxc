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

#ifndef KEEPASSXC_MOCKNEXTCLOUDSYNCPROVIDER_H
#define KEEPASSXC_MOCKNEXTCLOUDSYNCPROVIDER_H

#include "remotesync/NextcloudSyncProvider.h"

struct NextcloudSyncParams;

// Test double for NextcloudSyncProvider: replaces the network-fronted
// operations (download / upload / refreshAuth / testConnection) with canned
// successes. Drop-in via RemoteSyncProvider::setFactoryOverrideForTest -- the
// overrides never touch QNAM, so callers must NOT call setNetworkAccessManager
// on the mock.
//
// Mirrors MockDropboxSyncProvider's shape minus the revokeToken hook (Nextcloud
// Login Flow v2 has no server-side revoke endpoint -- onRemoveClicked clears
// local config only). Default behavior is "happy path for an already-authorized
// user":
//   * refreshAuth    -> success, empty stdOutput (no token rotation)
//   * download       -> success; filePath copied from s_downloadSourcePath if
//                       set, empty otherwise (= "remote file does not exist
//                       yet", SyncEngine takes the first-sync branch and skips
//                       merge)
//   * upload         -> success
//   * testConnection -> success; filePath echoed from s_downloadSourcePath if
//                       set (= "Nextcloud connection successful."), empty
//                       otherwise (= "Connected. File not found ...")
//
// Per-test customization is via the static setters below. Because the page and
// DatabaseWidget each create() their own provider instance, instance-level
// configuration would not reach both -- statics are the legitimate exemption
// (same rationale as MockDropboxSyncProvider).
class MockNextcloudSyncProvider : public NextcloudSyncProvider
{
    Q_OBJECT

public:
    explicit MockNextcloudSyncProvider(QObject* parent = nullptr);
    ~MockNextcloudSyncProvider() override = default;

    RemoteHandler::RemoteResult download(const RemoteSyncParams* params) override;
    RemoteHandler::RemoteResult upload(const QString& filePath, const RemoteSyncParams* params) override;
    RemoteHandler::RemoteResult refreshAuth(const RemoteSyncParams* params) override;
    RemoteHandler::RemoteResult testConnection(const NextcloudSyncParams* params) override;

    // Set the source .kdbx file that the NEXT download() / testConnection()
    // call will copy to a temp path and return. download() is one-shot
    // (consumes the source); testConnection() is NOT (it merely peeks at the
    // source, so a single set persists across multiple Test Connection clicks
    // until the test explicitly resets it). The asymmetry matches usage:
    // SyncEngine's chained-sync loop drives download() and would re-merge a
    // static canonical kdbx forever otherwise (see MockDropboxSyncProvider for
    // the same one-shot rationale).
    static void setDownloadSourcePath(const QString& path);

    // Set the next failure to return from download(). After being returned
    // once, the failure is cleared and subsequent calls revert to success.
    // Used to inject sync failure scenarios.
    static void setNextDownloadFailure(const QString& errorMessage,
                                       RemoteHandler::ErrorKind kind = RemoteHandler::ErrorKind::Other);

    // Test-only chain breaker for the post-sync re-trigger loop. When set to
    // false, isAuthorized() returns false unconditionally -- same rationale and
    // semantics as MockDropboxSyncProvider::setIsAuthorizedOverride. Default
    // true (matches production behavior).
    static void setIsAuthorizedOverride(bool authorized);

    bool isAuthorized(const QJsonObject& config) const override;

    // Call counters for assertions.
    static int downloadCallCount();
    static int uploadCallCount();
    static int refreshAuthCallCount();
    static int testConnectionCallCount();
    static void resetCallCounts();

private:
    static QString s_downloadSourcePath;
    static QString s_nextDownloadFailureMessage;
    static RemoteHandler::ErrorKind s_nextDownloadFailureKind;
    static int s_downloadCallCount;
    static int s_uploadCallCount;
    static int s_refreshAuthCallCount;
    static int s_testConnectionCallCount;
    static bool s_isAuthorizedOverride;
};

#endif // KEEPASSXC_MOCKNEXTCLOUDSYNCPROVIDER_H
