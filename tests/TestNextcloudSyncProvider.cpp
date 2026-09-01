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

#include "TestNextcloudSyncProvider.h"

#include "crypto/Crypto.h"
#include "remotesync/NextcloudSyncProvider.h"
#include "remotesync/RemoteSyncParams.h"

#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QScopedPointer>
#include <QString>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

using ServerUrlValidity = NextcloudSyncProvider::ServerUrlValidity;
using ErrorKind = RemoteSyncProvider::ErrorKind;

QTEST_GUILESS_MAIN(TestNextcloudSyncProvider)

void TestNextcloudSyncProvider::initTestCase()
{
    QVERIFY(Crypto::init());
}

// ---------------------------------------------------------------------------
// canonicalizeServerBaseUrl
// ---------------------------------------------------------------------------

void TestNextcloudSyncProvider::testCanonicalize_addsHttpsWhenSchemeAbsent()
{
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("cloud.example.com")),
             QStringLiteral("https://cloud.example.com"));
}

void TestNextcloudSyncProvider::testCanonicalize_schemelessHostPort()
{
    // Ensures "host:port" without a scheme is parsed as host and port,
    // not as scheme="host".
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("localhost:8080")),
             QStringLiteral("https://localhost:8080"));
}

void TestNextcloudSyncProvider::testCanonicalize_acceptsHttps()
{
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("https://cloud.example.com")),
             QStringLiteral("https://cloud.example.com"));
}

void TestNextcloudSyncProvider::testCanonicalize_acceptsHttpForLoopback()
{
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("http://localhost:8080")),
             QStringLiteral("http://localhost:8080"));
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("http://127.0.0.1")),
             QStringLiteral("http://127.0.0.1"));
    // IPv6 loopback. Qt URL serialization brackets the IPv6 host on output.
    const QString ipv6 = NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("http://[::1]"));
    QVERIFY2(!ipv6.isEmpty(), "IPv6 loopback (::1) must be accepted");
    QVERIFY2(ipv6.startsWith(QStringLiteral("http://")),
             "IPv6 loopback canonicalization must preserve http scheme");
    QVERIFY2(ipv6.contains(QStringLiteral("::1")),
             qPrintable(QStringLiteral("IPv6 host must survive canonicalization; got: %1").arg(ipv6)));
}

void TestNextcloudSyncProvider::testCanonicalize_rejectsHttpForNonLoopback()
{
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("http://cloud.example.com")), QString());
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("http://example.com")), QString());
    // 10.0.0.5 is private but not loopback.
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("http://10.0.0.5")), QString());
    // "localhost.evil.com" has "localhost" only as a label, not the entire host.
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("http://localhost.evil.com")), QString());
}

void TestNextcloudSyncProvider::testCanonicalize_rejectsNonHttpHttpsSchemes()
{
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("ftp://example.com")), QString());
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("file:///etc/passwd")), QString());
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("javascript:alert(1)")), QString());
}

void TestNextcloudSyncProvider::testCanonicalize_stripsTrailingSlash()
{
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("https://cloud.example.com/")),
             QStringLiteral("https://cloud.example.com"));
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("https://cloud.example.com///")),
             QStringLiteral("https://cloud.example.com"));
}

void TestNextcloudSyncProvider::testCanonicalize_preservesSubpath()
{
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("https://cloud.example.com/nextcloud")),
             QStringLiteral("https://cloud.example.com/nextcloud"));
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("https://cloud.example.com/nextcloud/")),
             QStringLiteral("https://cloud.example.com/nextcloud"));
}

void TestNextcloudSyncProvider::testCanonicalize_stripsFragmentAndQuery()
{
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("https://cloud.example.com?foo=bar")),
             QStringLiteral("https://cloud.example.com"));
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("https://cloud.example.com#hash")),
             QStringLiteral("https://cloud.example.com"));
}

void TestNextcloudSyncProvider::testCanonicalize_emptyInput()
{
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QString()), QString());
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("")), QString());
    QCOMPARE(NextcloudSyncProvider::canonicalizeServerBaseUrl(QStringLiteral("   ")), QString());
}

void TestNextcloudSyncProvider::testCanonicalize_idempotent()
{
    const QStringList samples = {
        QStringLiteral("cloud.example.com"),
        QStringLiteral("https://cloud.example.com/"),
        QStringLiteral("https://cloud.example.com/nextcloud/"),
        QStringLiteral("http://localhost:8080"),
        QStringLiteral("https://cloud.example.com?x=y#z"),
    };
    for (const QString& s : samples) {
        const QString once = NextcloudSyncProvider::canonicalizeServerBaseUrl(s);
        const QString twice = NextcloudSyncProvider::canonicalizeServerBaseUrl(once);
        QCOMPARE(twice, once);
    }
}

// ---------------------------------------------------------------------------
// isLoopbackHost
// ---------------------------------------------------------------------------

void TestNextcloudSyncProvider::testIsLoopbackHost_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<bool>("expected");

    // The function takes a QUrl, so we encode the host into one. Use the
    // http:// scheme so any host string is permissible to parse.
    QTest::newRow("localhost lower") << QStringLiteral("localhost") << true;
    QTest::newRow("LOCALHOST upper (case-insensitive)") << QStringLiteral("LOCALHOST") << true;
    QTest::newRow("127.0.0.1") << QStringLiteral("127.0.0.1") << true;
    QTest::newRow("127.255.255.255 (whole 127.0.0.0/8)") << QStringLiteral("127.255.255.255") << true;
    QTest::newRow("::1 IPv6 loopback") << QStringLiteral("[::1]") << true;
    QTest::newRow("localhost.example.com") << QStringLiteral("localhost.example.com") << false;
    QTest::newRow("cloud.example.com") << QStringLiteral("cloud.example.com") << false;
    QTest::newRow("10.0.0.1") << QStringLiteral("10.0.0.1") << false;
    QTest::newRow("192.168.1.1") << QStringLiteral("192.168.1.1") << false;
    QTest::newRow("empty host") << QString() << false;
}

void TestNextcloudSyncProvider::testIsLoopbackHost()
{
    QFETCH(QString, host);
    QFETCH(bool, expected);

    QUrl url;
    url.setScheme(QStringLiteral("http"));
    if (!host.isEmpty()) {
        // Strip outer brackets for IPv6 — QUrl::setHost wants the bare form.
        QString h = host;
        if (h.startsWith(QLatin1Char('[')) && h.endsWith(QLatin1Char(']'))) {
            h = h.mid(1, h.size() - 2);
        }
        url.setHost(h);
    }
    QCOMPARE(NextcloudSyncProvider::isLoopbackHost(url), expected);
}

// ---------------------------------------------------------------------------
// validateServerUrl
// ---------------------------------------------------------------------------

void TestNextcloudSyncProvider::testValidateServerUrl_empty()
{
    QString sentinel = QStringLiteral("SENTINEL_UNCHANGED");

    QString canon = sentinel;
    QCOMPARE(NextcloudSyncProvider::validateServerUrl(QString(), &canon), ServerUrlValidity::Empty);
    QCOMPARE(canon, sentinel);

    canon = sentinel;
    QCOMPARE(NextcloudSyncProvider::validateServerUrl(QStringLiteral("   "), &canon), ServerUrlValidity::Empty);
    QCOMPARE(canon, sentinel);
}

void TestNextcloudSyncProvider::testValidateServerUrl_notSecure_beforeMalformed()
{
    // The ordering is locked: NotSecure must be reported BEFORE syntactic
    // checks so the user sees the cleartext-policy banner rather than a
    // generic "invalid URL" for a syntactically-fine but insecure URL.
    QString sentinel = QStringLiteral("SENTINEL_UNCHANGED");
    QString canon = sentinel;
    QCOMPARE(NextcloudSyncProvider::validateServerUrl(QStringLiteral("http://example.com"), &canon),
             ServerUrlValidity::NotSecure);
    QCOMPARE(canon, sentinel);
}

void TestNextcloudSyncProvider::testValidateServerUrl_notSecure_loopbackOk()
{
    QCOMPARE(NextcloudSyncProvider::validateServerUrl(QStringLiteral("http://localhost")), ServerUrlValidity::Ok);
}

void TestNextcloudSyncProvider::testValidateServerUrl_malformed_unsupportedScheme()
{
    QCOMPARE(NextcloudSyncProvider::validateServerUrl(QStringLiteral("ftp://example.com")),
             ServerUrlValidity::Malformed);
}

void TestNextcloudSyncProvider::testValidateServerUrl_malformed_noHost()
{
    QCOMPARE(NextcloudSyncProvider::validateServerUrl(QStringLiteral("https://")), ServerUrlValidity::Malformed);
    QCOMPARE(NextcloudSyncProvider::validateServerUrl(QStringLiteral("https:///path")), ServerUrlValidity::Malformed);
}

void TestNextcloudSyncProvider::testValidateServerUrl_ok_fillsCanonicalOut()
{
    QString canon;
    QCOMPARE(NextcloudSyncProvider::validateServerUrl(QStringLiteral("cloud.example.com"), &canon),
             ServerUrlValidity::Ok);
    QCOMPARE(canon, QStringLiteral("https://cloud.example.com"));
}

// ---------------------------------------------------------------------------
// normalizeRemotePath
// ---------------------------------------------------------------------------

void TestNextcloudSyncProvider::testNormalizeRemotePath_trimsWhitespace()
{
    QCOMPARE(NextcloudSyncProvider::normalizeRemotePath(QStringLiteral("  /foo.kdbx  ")),
             QStringLiteral("/foo.kdbx"));
}

void TestNextcloudSyncProvider::testNormalizeRemotePath_NFC()
{
    // Decomposed form: 'e' + U+0301 (COMBINING ACUTE ACCENT)
    QString decomposed = QStringLiteral("/caf");
    decomposed.append(QChar(0x0065)); // 'e'
    decomposed.append(QChar(0x0301)); // combining acute
    decomposed.append(QStringLiteral(".kdbx"));

    // Precomposed form: U+00E9 'é'
    QString precomposed = QStringLiteral("/caf");
    precomposed.append(QChar(0x00E9));
    precomposed.append(QStringLiteral(".kdbx"));

    // Sanity: the two encodings must not be byte-equal before normalization.
    QVERIFY(decomposed != precomposed);

    const QString normalized = NextcloudSyncProvider::normalizeRemotePath(decomposed);
    QCOMPARE(QString::compare(normalized, precomposed, Qt::CaseSensitive), 0);
}

void TestNextcloudSyncProvider::testNormalizeRemotePath_idempotent()
{
    const QStringList samples = {
        QStringLiteral("/foo.kdbx"),
        QStringLiteral("  /spaced.kdbx  "),
        QStringLiteral("/café.kdbx"),
        QStringLiteral("/folder/sub/db.kdbx"),
    };
    for (const QString& s : samples) {
        const QString once = NextcloudSyncProvider::normalizeRemotePath(s);
        const QString twice = NextcloudSyncProvider::normalizeRemotePath(once);
        QCOMPARE(twice, once);
    }
}

// ---------------------------------------------------------------------------
// buildResourceUrl
// ---------------------------------------------------------------------------

void TestNextcloudSyncProvider::testBuildResourceUrl_composesCorrectly()
{
    const QUrl url = NextcloudSyncProvider::buildResourceUrl(
        QStringLiteral("https://cloud.example.com"), QStringLiteral("alice"), QStringLiteral("/foo.kdbx"));
    QCOMPARE(url.toString(QUrl::FullyEncoded),
             QStringLiteral("https://cloud.example.com/remote.php/dav/files/alice/foo.kdbx"));
}

void TestNextcloudSyncProvider::testBuildResourceUrl_encodesLoginNameAtSign()
{
    const QUrl url = NextcloudSyncProvider::buildResourceUrl(QStringLiteral("https://cloud.example.com"),
                                                             QStringLiteral("alice@example.com"),
                                                             QStringLiteral("/foo.kdbx"));
    const QString s = url.toString(QUrl::FullyEncoded);
    QVERIFY2(s.contains(QStringLiteral("/remote.php/dav/files/alice%40example.com/")),
             qPrintable(QStringLiteral("Expected %40 (encoded '@') in login segment; got: %1").arg(s)));
}

void TestNextcloudSyncProvider::testBuildResourceUrl_preservesSubpath()
{
    const QUrl url = NextcloudSyncProvider::buildResourceUrl(
        QStringLiteral("https://cloud.example.com/nextcloud"), QStringLiteral("alice"), QStringLiteral("/foo.kdbx"));
    QCOMPARE(url.toString(QUrl::FullyEncoded),
             QStringLiteral("https://cloud.example.com/nextcloud/remote.php/dav/files/alice/foo.kdbx"));
}

void TestNextcloudSyncProvider::testBuildResourceUrl_encodesSpacesInRemotePath()
{
    {
        const QUrl url = NextcloudSyncProvider::buildResourceUrl(QStringLiteral("https://cloud.example.com"),
                                                                 QStringLiteral("alice"),
                                                                 QStringLiteral("/my passwords.kdbx"));
        const QString s = url.toString(QUrl::FullyEncoded);
        QVERIFY2(s.contains(QStringLiteral("/my%20passwords.kdbx")),
                 qPrintable(QStringLiteral("Expected '%20' for space; got: %1").arg(s)));
    }
    {
        QString remotePath = QStringLiteral("/caf");
        remotePath.append(QChar(0x00E9)); // é (precomposed)
        remotePath.append(QStringLiteral(".kdbx"));
        const QUrl url = NextcloudSyncProvider::buildResourceUrl(
            QStringLiteral("https://cloud.example.com"), QStringLiteral("alice"), remotePath);
        const QString s = url.toString(QUrl::FullyEncoded);
        QVERIFY2(s.contains(QStringLiteral("/caf%C3%A9.kdbx")),
                 qPrintable(QStringLiteral("Expected UTF-8 %%C3%%A9 for 'é'; got: %1").arg(s)));
    }
}

void TestNextcloudSyncProvider::testBuildResourceUrl_remotePathWithoutLeadingSlash_addsOne()
{
    const QUrl url = NextcloudSyncProvider::buildResourceUrl(
        QStringLiteral("https://cloud.example.com"), QStringLiteral("alice"), QStringLiteral("foo.kdbx"));
    const QString s = url.toString(QUrl::FullyEncoded);
    QCOMPARE(s, QStringLiteral("https://cloud.example.com/remote.php/dav/files/alice/foo.kdbx"));
    QVERIFY2(!s.contains(QStringLiteral("//foo.kdbx")), "must not double-slash before the filename");
}

// ---------------------------------------------------------------------------
// buildParamsFromConfig
// ---------------------------------------------------------------------------

void TestNextcloudSyncProvider::testBuildParamsFromConfig_extractsAllFields()
{
    NextcloudSyncProvider provider;
    QJsonObject config;
    config.insert(QStringLiteral("serverBaseUrl"), QStringLiteral("https://cloud.example.com"));
    config.insert(QStringLiteral("remotePath"), QStringLiteral("/Passwords/db.kdbx"));
    config.insert(QStringLiteral("loginName"), QStringLiteral("alice"));
    config.insert(QStringLiteral("appPassword"), QStringLiteral("hunter2-app-password"));
    config.insert(QStringLiteral("timeoutMsec"), 15000);

    QScopedPointer<RemoteSyncParams> base(provider.buildParamsFromConfig(config));
    QVERIFY(base);
    QCOMPARE(base->type, QStringLiteral("nextcloud"));
    auto* p = dynamic_cast<NextcloudSyncParams*>(base.data());
    QVERIFY2(p, "buildParamsFromConfig must return a NextcloudSyncParams");
    QCOMPARE(p->serverBaseUrl, QStringLiteral("https://cloud.example.com"));
    QCOMPARE(p->remotePath, QStringLiteral("/Passwords/db.kdbx"));
    QCOMPARE(p->loginName, QStringLiteral("alice"));
    QCOMPARE(p->appPassword, QStringLiteral("hunter2-app-password"));
    QCOMPARE(p->timeoutMsec, 15000);
}

void TestNextcloudSyncProvider::testBuildParamsFromConfig_timeoutMsec_default()
{
    NextcloudSyncProvider provider;
    QJsonObject config;
    config.insert(QStringLiteral("serverBaseUrl"), QStringLiteral("https://cloud.example.com"));
    config.insert(QStringLiteral("remotePath"), QStringLiteral("/db.kdbx"));
    config.insert(QStringLiteral("loginName"), QStringLiteral("alice"));
    config.insert(QStringLiteral("appPassword"), QStringLiteral("pw"));
    // timeoutMsec deliberately omitted.

    QScopedPointer<RemoteSyncParams> base(provider.buildParamsFromConfig(config));
    auto* p = dynamic_cast<NextcloudSyncParams*>(base.data());
    QVERIFY(p);
    QCOMPARE(p->timeoutMsec, 30000);
}

// ---------------------------------------------------------------------------
// classifyError -- substring dispatch on locked banner fragments. The
// fragments below match the banner strings in NextcloudSyncProvider.cpp;
// the test locks the keyword -> ErrorKind mapping byte-for-byte.
// ---------------------------------------------------------------------------

void TestNextcloudSyncProvider::testClassifyError_authVariants()
{
    NextcloudSyncProvider provider;

    // 401 "Nextcloud authorization expired..." -> AuthExpired
    QCOMPARE(provider.classifyError(QStringLiteral(
                 "Nextcloud authorization expired. Re-authorize in Database > Settings > Cloud Sync.")),
             ErrorKind::AuthExpired);
    // Case-insensitive substring match.
    QCOMPARE(provider.classifyError(QStringLiteral("AUTHORIZATION EXPIRED")), ErrorKind::AuthExpired);

    // Manual-paste credential-rejection banner -> AuthExpired (intentional collapse).
    QCOMPARE(provider.classifyError(QStringLiteral(
                 "Nextcloud rejected those credentials. Verify the username and app password.")),
             ErrorKind::AuthExpired);

    // 403 -> Permission
    QCOMPARE(provider.classifyError(QStringLiteral(
                 "Nextcloud denied access to this path. Verify the file path and your account permissions.")),
             ErrorKind::Permission);

    // 404 trash banner -> NotFound
    QCOMPARE(provider.classifyError(QStringLiteral(
                 "Database is in your Nextcloud trash. Restore it from Nextcloud Files, then try syncing again.")),
             ErrorKind::NotFound);
    // 404 testConnection banner -> NotFound (separate fragment)
    QCOMPARE(provider.classifyError(QStringLiteral(
                 "Nextcloud could not find the configured remote path. Verify your settings.")),
             ErrorKind::NotFound);

    // 412 -> Conflict
    QCOMPARE(provider.classifyError(QStringLiteral(
                 "Remote file changed since last download. Re-sync to merge changes.")),
             ErrorKind::Conflict);

    // 423 -> RateLimit
    QCOMPARE(provider.classifyError(QStringLiteral("Nextcloud file is locked. Try again in a moment.")),
             ErrorKind::RateLimit);

    // 507 -> Quota
    QCOMPARE(provider.classifyError(QStringLiteral("Nextcloud server is out of storage. Free space and try again.")),
             ErrorKind::Quota);

    // SSL handshake -> Network (no dedicated SslHandshake kind)
    QCOMPARE(provider.classifyError(QStringLiteral(
                 "Nextcloud server's SSL certificate could not be verified. "
                 "Check that your server's certificate is valid and the chain is correctly configured.")),
             ErrorKind::Network);

    // 5xx generic -> ServerError
    QCOMPARE(provider.classifyError(QStringLiteral("Nextcloud server error (HTTP 502). Try again later.")),
             ErrorKind::ServerError);

    // Unknown -> Other
    QCOMPARE(provider.classifyError(QStringLiteral("something completely unrelated")), ErrorKind::Other);
}

// ---------------------------------------------------------------------------
// isAuthorized -- four required fields. Data-driven: drop each one in turn,
// assert isAuthorized returns false; all-present returns true.
// ---------------------------------------------------------------------------

void TestNextcloudSyncProvider::testIsAuthorized_table_data()
{
    QTest::addColumn<bool>("hasServer");
    QTest::addColumn<bool>("hasLogin");
    QTest::addColumn<bool>("hasPassword");
    QTest::addColumn<bool>("hasRemotePath");
    QTest::addColumn<bool>("expected");

    QTest::newRow("all present") << true << true << true << true << true;
    QTest::newRow("missing serverBaseUrl") << false << true << true << true << false;
    QTest::newRow("missing loginName") << true << false << true << true << false;
    QTest::newRow("missing appPassword") << true << true << false << true << false;
    QTest::newRow("missing remotePath") << true << true << true << false << false;
    QTest::newRow("all missing") << false << false << false << false << false;
}

void TestNextcloudSyncProvider::testIsAuthorized_table()
{
    QFETCH(bool, hasServer);
    QFETCH(bool, hasLogin);
    QFETCH(bool, hasPassword);
    QFETCH(bool, hasRemotePath);
    QFETCH(bool, expected);

    NextcloudSyncProvider provider;
    QJsonObject config;
    if (hasServer) {
        config.insert(QStringLiteral("serverBaseUrl"), QStringLiteral("https://cloud.example.com"));
    }
    if (hasLogin) {
        config.insert(QStringLiteral("loginName"), QStringLiteral("alice"));
    }
    if (hasPassword) {
        config.insert(QStringLiteral("appPassword"), QStringLiteral("pw"));
    }
    if (hasRemotePath) {
        config.insert(QStringLiteral("remotePath"), QStringLiteral("/db.kdbx"));
    }
    QCOMPARE(provider.isAuthorized(config), expected);
}

// ---------------------------------------------------------------------------
// Entry-point validation -- these must reject BEFORE touching the network.
// We intentionally do NOT inject a QNetworkAccessManager: if validation lets
// the call fall through to ensureNam() / m_nam->get(), the test would either
// segfault (no NAM) or attempt a real DNS lookup, neither of which are what
// the public contract promises.
// ---------------------------------------------------------------------------

void TestNextcloudSyncProvider::testDownload_rejectsEmptyServerBaseUrl()
{
    NextcloudSyncProvider provider;
    NextcloudSyncParams params;
    params.type = QStringLiteral("nextcloud");
    params.serverBaseUrl = QString();
    params.loginName = QStringLiteral("alice");
    params.appPassword = QStringLiteral("pw");
    params.remotePath = QStringLiteral("/db.kdbx");

    const auto result = provider.download(&params);
    QVERIFY(!result.success);
    QVERIFY2(result.errorMessage.contains(QStringLiteral("Nextcloud server URL"), Qt::CaseInsensitive),
             qPrintable(QStringLiteral("Expected server-URL error; got: %1").arg(result.errorMessage)));
}

void TestNextcloudSyncProvider::testDownload_rejectsEmptyLoginName()
{
    NextcloudSyncProvider provider;
    NextcloudSyncParams params;
    params.type = QStringLiteral("nextcloud");
    params.serverBaseUrl = QStringLiteral("https://cloud.example.com");
    params.loginName = QString();
    params.appPassword = QStringLiteral("pw");
    params.remotePath = QStringLiteral("/db.kdbx");

    const auto result = provider.download(&params);
    QVERIFY(!result.success);
    QVERIFY2(result.errorMessage.contains(QStringLiteral("login name"), Qt::CaseInsensitive),
             qPrintable(QStringLiteral("Expected login-name error; got: %1").arg(result.errorMessage)));
}

void TestNextcloudSyncProvider::testDownload_rejectsRelativeRemotePath()
{
    NextcloudSyncProvider provider;
    NextcloudSyncParams params;
    params.type = QStringLiteral("nextcloud");
    params.serverBaseUrl = QStringLiteral("https://cloud.example.com");
    params.loginName = QStringLiteral("alice");
    params.appPassword = QStringLiteral("pw");
    params.remotePath = QStringLiteral("foo.kdbx"); // no leading '/'

    const auto downloadResult = provider.download(&params);
    QVERIFY(!downloadResult.success);
    QVERIFY2(downloadResult.errorMessage.contains(QStringLiteral("must start with '/'")),
             qPrintable(QStringLiteral("Expected 'must start with /' error; got: %1").arg(downloadResult.errorMessage)));

    // Equivalent for upload: same validation, surfaced from uploadImpl.
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString localPath = tmpDir.path() + QStringLiteral("/db.kdbx");
    {
        QFile f(localPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("payload");
        f.close();
    }
    const auto uploadResult = provider.upload(localPath, &params);
    QVERIFY(!uploadResult.success);
    QVERIFY2(uploadResult.errorMessage.contains(QStringLiteral("must start with '/'")),
             qPrintable(QStringLiteral("Expected 'must start with /' error from upload; got: %1")
                            .arg(uploadResult.errorMessage)));
}

void TestNextcloudSyncProvider::testUpload_rejectsEmptyServerBaseUrl()
{
    NextcloudSyncProvider provider;
    NextcloudSyncParams params;
    params.type = QStringLiteral("nextcloud");
    params.serverBaseUrl = QString();
    params.loginName = QStringLiteral("alice");
    params.appPassword = QStringLiteral("pw");
    params.remotePath = QStringLiteral("/db.kdbx");

    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString localPath = tmpDir.path() + QStringLiteral("/db.kdbx");
    {
        QFile f(localPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("payload");
        f.close();
    }

    const auto result = provider.upload(localPath, &params);
    QVERIFY(!result.success);
    QVERIFY2(result.errorMessage.contains(QStringLiteral("Nextcloud server URL"), Qt::CaseInsensitive),
             qPrintable(QStringLiteral("Expected server-URL error; got: %1").arg(result.errorMessage)));
}

void TestNextcloudSyncProvider::testUpload_rejectsMissingFile()
{
    NextcloudSyncProvider provider;
    NextcloudSyncParams params;
    params.type = QStringLiteral("nextcloud");
    params.serverBaseUrl = QStringLiteral("https://cloud.example.com");
    params.loginName = QStringLiteral("alice");
    params.appPassword = QStringLiteral("pw");
    params.remotePath = QStringLiteral("/db.kdbx");

    // Path under a temp dir that we never create -- guaranteed not to exist.
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString nonexistent = tmpDir.path() + QStringLiteral("/does-not-exist.kdbx");
    QVERIFY(!QFile::exists(nonexistent));

    const auto result = provider.upload(nonexistent, &params);
    QVERIFY(!result.success);
    // upload returns "Failed to open file for upload: <path>" when QFile::open fails.
    QVERIFY2(result.errorMessage.contains(QStringLiteral("Failed to open file"), Qt::CaseInsensitive),
             qPrintable(QStringLiteral("Expected open-failed error; got: %1").arg(result.errorMessage)));
}
