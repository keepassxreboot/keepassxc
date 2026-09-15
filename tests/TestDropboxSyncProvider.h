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

#ifndef KEEPASSXC_TESTDROPBOXSYNCPROVIDER_H
#define KEEPASSXC_TESTDROPBOXSYNCPROVIDER_H

#include <QObject>

class TestDropboxSyncProvider : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // buildParamsFromConfig
    void testBuildParamsFromConfig_extractsAllFields();
    void testBuildParamsFromConfig_missingExpiresAt();

    // applyRefreshedTokens
    void testApplyRefreshedTokens_updatesAccessTokenAndExpiry();
    void testApplyRefreshedTokens_emptyStdOutputReturnsTrue();
    void testApplyRefreshedTokens_malformedJsonReturnsFalse();

    // classifyError
    void testClassifyError_invalidAccessToken_AuthExpired();
    void testClassifyError_invalidGrant_AuthRevoked();
    void testClassifyError_unknown_Other();

    // isAuthorized
    void testIsAuthorized_requiresAllFourFields();

    // Entry-point validation (no network)
    void testDownload_rejectsRelativeRemotePath();
    void testUpload_rejectsRelativeRemotePath();
    void testUpload_rejectsMissingFile();

    // refreshAuth early-return paths
    void testRefreshAuth_emptyRefreshToken_returnsAuthRevoked();
    void testRefreshAuth_validTokenWithinBuffer_skipsRefresh();

    // persistRefreshedTokens
    void testPersistRefreshedTokens_updatesAccessTokenOnly();
    void testPersistRefreshedTokens_noCloudConfig_noopWithWarning();
    void testPersistRefreshedTokens_wrongProviderType_noopWithWarning();
    void testPersistRefreshedTokens_malformedJson_noopWithWarning();
};

#endif // KEEPASSXC_TESTDROPBOXSYNCPROVIDER_H
