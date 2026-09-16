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

#ifndef KEEPASSXC_TESTNEXTCLOUDSYNCPROVIDER_H
#define KEEPASSXC_TESTNEXTCLOUDSYNCPROVIDER_H

#include <QObject>

class TestNextcloudSyncProvider : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // canonicalizeServerBaseUrl
    void testCanonicalize_addsHttpsWhenSchemeAbsent();
    void testCanonicalize_schemelessHostPort();
    void testCanonicalize_acceptsHttps();
    void testCanonicalize_acceptsHttpForLoopback();
    void testCanonicalize_rejectsHttpForNonLoopback();
    void testCanonicalize_rejectsNonHttpHttpsSchemes();
    void testCanonicalize_stripsTrailingSlash();
    void testCanonicalize_preservesSubpath();
    void testCanonicalize_stripsFragmentAndQuery();
    void testCanonicalize_emptyInput();
    void testCanonicalize_idempotent();

    // isLoopbackHost
    void testIsLoopbackHost_data();
    void testIsLoopbackHost();

    // validateServerUrl
    void testValidateServerUrl_empty();
    void testValidateServerUrl_notSecure_beforeMalformed();
    void testValidateServerUrl_notSecure_loopbackOk();
    void testValidateServerUrl_malformed_unsupportedScheme();
    void testValidateServerUrl_malformed_noHost();
    void testValidateServerUrl_ok_fillsCanonicalOut();

    // normalizeRemotePath
    void testNormalizeRemotePath_trimsWhitespace();
    void testNormalizeRemotePath_NFC();
    void testNormalizeRemotePath_idempotent();

    // buildResourceUrl
    void testBuildResourceUrl_composesCorrectly();
    void testBuildResourceUrl_encodesLoginNameAtSign();
    void testBuildResourceUrl_preservesSubpath();
    void testBuildResourceUrl_encodesSpacesInRemotePath();
    void testBuildResourceUrl_remotePathWithoutLeadingSlash_addsOne();

    // buildParamsFromConfig
    void testBuildParamsFromConfig_extractsAllFields();
    void testBuildParamsFromConfig_timeoutMsec_default();

    // classifyError
    void testClassifyError_authVariants();

    // isAuthorized
    void testIsAuthorized_table_data();
    void testIsAuthorized_table();

    // Entry-point validation (no network)
    void testDownload_rejectsEmptyServerBaseUrl();
    void testDownload_rejectsEmptyLoginName();
    void testDownload_rejectsRelativeRemotePath();
    void testUpload_rejectsEmptyServerBaseUrl();
    void testUpload_rejectsMissingFile();
};

#endif // KEEPASSXC_TESTNEXTCLOUDSYNCPROVIDER_H
