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

#include "TestDropboxSyncProvider.h"

#include "mock/MockClock.h"

#include "core/Clock.h"
#include "core/Database.h"
#include "crypto/Crypto.h"
#include "gui/remote/RemoteHandler.h"
#include "gui/remote/RemoteSettings.h"
#include "remotesync/DropboxSyncProvider.h"
#include "remotesync/RemoteSyncParams.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopedPointer>
#include <QString>
#include <QTest>

QTEST_GUILESS_MAIN(TestDropboxSyncProvider)

void TestDropboxSyncProvider::initTestCase()
{
    QVERIFY(Crypto::init());
}

// ---------------------------------------------------------------------------
// buildParamsFromConfig
// ---------------------------------------------------------------------------

void TestDropboxSyncProvider::testBuildParamsFromConfig_extractsAllFields()
{
    DropboxSyncProvider provider;
    const qint64 expiresMs = QDateTime(QDate(2030, 1, 1), QTime(12, 0, 0), Qt::UTC).toMSecsSinceEpoch();

    QJsonObject config;
    config[QStringLiteral("name")] = QStringLiteral("My Dropbox");
    config[QStringLiteral("appKey")] = QStringLiteral("abc123");
    config[QStringLiteral("remotePath")] = QStringLiteral("/Apps/KeePassXC/db.kdbx");
    config[QStringLiteral("accessToken")] = QStringLiteral("at-token");
    config[QStringLiteral("refreshToken")] = QStringLiteral("rt-token");
    config[QStringLiteral("expiresAt")] = expiresMs;

    QScopedPointer<RemoteSyncParams> params(provider.buildParamsFromConfig(config));
    QVERIFY(params);
    auto* dpx = dynamic_cast<DropboxSyncParams*>(params.data());
    QVERIFY(dpx);
    QCOMPARE(dpx->type, QStringLiteral("dropbox"));
    QCOMPARE(dpx->name, QStringLiteral("My Dropbox"));
    QCOMPARE(dpx->appKey, QStringLiteral("abc123"));
    QCOMPARE(dpx->remotePath, QStringLiteral("/Apps/KeePassXC/db.kdbx"));
    QCOMPARE(dpx->accessToken, QStringLiteral("at-token"));
    QCOMPARE(dpx->refreshToken, QStringLiteral("rt-token"));
    QVERIFY(dpx->expiresAt.isValid());
    QCOMPARE(dpx->expiresAt.toMSecsSinceEpoch(), expiresMs);
}

void TestDropboxSyncProvider::testBuildParamsFromConfig_missingExpiresAt()
{
    DropboxSyncProvider provider;
    QJsonObject config;
    config[QStringLiteral("name")] = QStringLiteral("My Dropbox");
    config[QStringLiteral("appKey")] = QStringLiteral("abc123");
    config[QStringLiteral("remotePath")] = QStringLiteral("/db.kdbx");
    config[QStringLiteral("accessToken")] = QStringLiteral("at");
    config[QStringLiteral("refreshToken")] = QStringLiteral("rt");
    // expiresAt deliberately omitted

    QScopedPointer<RemoteSyncParams> params(provider.buildParamsFromConfig(config));
    QVERIFY(params);
    auto* dpx = dynamic_cast<DropboxSyncParams*>(params.data());
    QVERIFY(dpx);
    // Absent expiresAt must remain invalid (not silently populated from 0 ms).
    QVERIFY(!dpx->expiresAt.isValid());
}

// ---------------------------------------------------------------------------
// applyRefreshedTokens
// ---------------------------------------------------------------------------

void TestDropboxSyncProvider::testApplyRefreshedTokens_updatesAccessTokenAndExpiry()
{
    DropboxSyncProvider provider;
    DropboxSyncParams params;
    params.type = QStringLiteral("dropbox");
    params.accessToken = QStringLiteral("old-access");
    params.refreshToken = QStringLiteral("kept-refresh");
    params.expiresAt = QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0), Qt::UTC);

    const qint64 newExpiresMs = QDateTime(QDate(2030, 1, 1), QTime(12, 0, 0), Qt::UTC).toMSecsSinceEpoch();
    QJsonObject tokenData;
    tokenData[QStringLiteral("accessToken")] = QStringLiteral("new-access");
    tokenData[QStringLiteral("expiresAt")] = newExpiresMs;
    const QString stdOutput = QString::fromUtf8(QJsonDocument(tokenData).toJson(QJsonDocument::Compact));

    QVERIFY(provider.applyRefreshedTokens(stdOutput, &params));
    QCOMPARE(params.accessToken, QStringLiteral("new-access"));
    QVERIFY(params.expiresAt.isValid());
    QCOMPARE(params.expiresAt.toMSecsSinceEpoch(), newExpiresMs);
    // CRITICAL: Dropbox does not return refresh_token on refresh; the existing
    // refreshToken must survive applyRefreshedTokens unchanged. A regression
    // here breaks long-term auth (cannot refresh again after access expires).
    QCOMPARE(params.refreshToken, QStringLiteral("kept-refresh"));
}

void TestDropboxSyncProvider::testApplyRefreshedTokens_emptyStdOutputReturnsTrue()
{
    DropboxSyncProvider provider;
    DropboxSyncParams params;
    params.accessToken = QStringLiteral("unchanged");
    params.refreshToken = QStringLiteral("rt");
    // Empty stdOutput is the proactive-refresh-skipped path: must succeed
    // as a no-op without touching params.
    QVERIFY(provider.applyRefreshedTokens(QString(), &params));
    QCOMPARE(params.accessToken, QStringLiteral("unchanged"));
    QCOMPARE(params.refreshToken, QStringLiteral("rt"));
}

void TestDropboxSyncProvider::testApplyRefreshedTokens_malformedJsonReturnsFalse()
{
    DropboxSyncProvider provider;
    DropboxSyncParams params;
    params.accessToken = QStringLiteral("untouched");
    QTest::ignoreMessage(QtWarningMsg, "DropboxSyncProvider: failed to parse refreshed token JSON");
    QVERIFY(!provider.applyRefreshedTokens(QStringLiteral("not json{"), &params));
    QCOMPARE(params.accessToken, QStringLiteral("untouched"));
}

// ---------------------------------------------------------------------------
// classifyError
// ---------------------------------------------------------------------------

void TestDropboxSyncProvider::testClassifyError_invalidAccessToken_AuthExpired()
{
    DropboxSyncProvider provider;
    QCOMPARE(provider.classifyError(QStringLiteral("invalid_access_token")),
             RemoteSyncProvider::ErrorKind::AuthExpired);
    QCOMPARE(provider.classifyError(QStringLiteral("expired_access_token")),
             RemoteSyncProvider::ErrorKind::AuthExpired);
    // Case-insensitivity guard -- Dropbox error tags may arrive in any case.
    QCOMPARE(provider.classifyError(QStringLiteral("INVALID_ACCESS_TOKEN")),
             RemoteSyncProvider::ErrorKind::AuthExpired);
}

void TestDropboxSyncProvider::testClassifyError_invalidGrant_AuthRevoked()
{
    DropboxSyncProvider provider;
    QCOMPARE(provider.classifyError(QStringLiteral("invalid_grant")),
             RemoteSyncProvider::ErrorKind::AuthRevoked);
}

void TestDropboxSyncProvider::testClassifyError_unknown_Other()
{
    DropboxSyncProvider provider;
    QCOMPARE(provider.classifyError(QStringLiteral("totally random error")),
             RemoteSyncProvider::ErrorKind::Other);
}

// ---------------------------------------------------------------------------
// isAuthorized
// ---------------------------------------------------------------------------

void TestDropboxSyncProvider::testIsAuthorized_requiresAllFourFields()
{
    DropboxSyncProvider provider;

    auto fullConfig = []() {
        QJsonObject c;
        c[QStringLiteral("accessToken")] = QStringLiteral("at");
        c[QStringLiteral("refreshToken")] = QStringLiteral("rt");
        c[QStringLiteral("appKey")] = QStringLiteral("ak");
        c[QStringLiteral("remotePath")] = QStringLiteral("/db.kdbx");
        return c;
    };

    // Fully populated config is authorized.
    QVERIFY(provider.isAuthorized(fullConfig()));

    // Each individual missing field must flip the verdict to false. Tests
    // the AND-of-four contract documented in isAuthorized().
    for (const QString& missing :
         {QStringLiteral("accessToken"),
          QStringLiteral("refreshToken"),
          QStringLiteral("appKey"),
          QStringLiteral("remotePath")}) {
        QJsonObject c = fullConfig();
        c[missing] = QStringLiteral("");
        QVERIFY2(!provider.isAuthorized(c),
                 qPrintable(QStringLiteral("expected unauthorized when '%1' missing").arg(missing)));
    }
}

// ---------------------------------------------------------------------------
// Entry-point validation (no network)
// ---------------------------------------------------------------------------

void TestDropboxSyncProvider::testDownload_rejectsRelativeRemotePath()
{
    DropboxSyncProvider provider;
    DropboxSyncParams params;
    params.type = QStringLiteral("dropbox");
    params.accessToken = QStringLiteral("at");
    params.refreshToken = QStringLiteral("rt");
    params.appKey = QStringLiteral("ak");
    params.remotePath = QStringLiteral("no-leading-slash");

    // Validation must happen before ensureNam() -- if QNAM is touched here,
    // the test would either hit the network or fail later. We assert the
    // synchronous validation rejection.
    const auto result = provider.download(&params);
    QVERIFY(!result.success);
    QVERIFY2(result.errorMessage.contains(QStringLiteral("must start with '/'")),
             qPrintable(result.errorMessage));
}

void TestDropboxSyncProvider::testUpload_rejectsRelativeRemotePath()
{
    DropboxSyncProvider provider;
    DropboxSyncParams params;
    params.type = QStringLiteral("dropbox");
    params.accessToken = QStringLiteral("at");
    params.remotePath = QStringLiteral("relative/path.kdbx");

    const auto result = provider.upload(QStringLiteral("/tmp/whatever.kdbx"), &params);
    QVERIFY(!result.success);
    QVERIFY2(result.errorMessage.contains(QStringLiteral("must start with '/'")),
             qPrintable(result.errorMessage));
}

void TestDropboxSyncProvider::testUpload_rejectsMissingFile()
{
    DropboxSyncProvider provider;
    DropboxSyncParams params;
    params.type = QStringLiteral("dropbox");
    params.accessToken = QStringLiteral("at");
    params.remotePath = QStringLiteral("/foo.kdbx");

    const QString missingFile = QStringLiteral("/this/path/should/not/exist/db.kdbx");
    const auto result = provider.upload(missingFile, &params);
    QVERIFY(!result.success);
    QVERIFY2(result.errorMessage.contains(QStringLiteral("Failed to open file")),
             qPrintable(result.errorMessage));
}

// ---------------------------------------------------------------------------
// refreshAuth early-return paths
// ---------------------------------------------------------------------------

void TestDropboxSyncProvider::testRefreshAuth_emptyRefreshToken_returnsAuthRevoked()
{
    DropboxSyncProvider provider;
    DropboxSyncParams params;
    params.type = QStringLiteral("dropbox");
    params.refreshToken = QString(); // empty
    params.appKey = QStringLiteral("ak");

    const auto result = provider.refreshAuth(&params);
    QVERIFY(!result.success);
    QCOMPARE(result.kind, RemoteSyncProvider::ErrorKind::AuthRevoked);
}

void TestDropboxSyncProvider::testRefreshAuth_validTokenWithinBuffer_skipsRefresh()
{
    // Pin the clock so the proactive-skip window is deterministic. The
    // skip path returns success without touching QNAM; this locks in that
    // optimization (regression: an extra HTTP call per sync would burn
    // Dropbox rate-limit headroom).
    auto* clock = new MockClock(2025, 1, 1, 12, 0, 0);
    MockClock::setup(clock);

    DropboxSyncProvider provider;
    DropboxSyncParams params;
    params.type = QStringLiteral("dropbox");
    params.refreshToken = QStringLiteral("rt");
    params.appKey = QStringLiteral("ak");
    // 1 hour ahead -- well outside the 10-minute proactive buffer.
    params.expiresAt = Clock::currentDateTimeUtc().addSecs(3600);

    const auto result = provider.refreshAuth(&params);
    QVERIFY(result.success);
    QVERIFY(result.stdOutput.isEmpty()); // empty stdOutput is the skip signal

    MockClock::teardown();
}

// ---------------------------------------------------------------------------
// persistRefreshedTokens
// ---------------------------------------------------------------------------

void TestDropboxSyncProvider::testPersistRefreshedTokens_updatesAccessTokenOnly()
{
    DropboxSyncProvider provider;
    RemoteSettings settings(QSharedPointer<Database>(), nullptr);

    const QString configKey = QStringLiteral("myKey");
    const qint64 oldExpires = QDateTime(QDate(2020, 1, 1), QTime(0, 0, 0), Qt::UTC).toMSecsSinceEpoch();
    const qint64 newExpires = QDateTime(QDate(2030, 6, 6), QTime(6, 6, 6), Qt::UTC).toMSecsSinceEpoch();

    QJsonObject existing;
    existing[QStringLiteral("type")] = QStringLiteral("dropbox");
    existing[QStringLiteral("name")] = configKey;
    existing[QStringLiteral("appKey")] = QStringLiteral("ak-original");
    existing[QStringLiteral("remotePath")] = QStringLiteral("/db.kdbx");
    existing[QStringLiteral("accessToken")] = QStringLiteral("old-at");
    existing[QStringLiteral("refreshToken")] = QStringLiteral("rt-must-survive");
    existing[QStringLiteral("expiresAt")] = oldExpires;
    settings.setProviderConfig(QStringLiteral("dropbox"), configKey, existing);

    QJsonObject tokenData;
    tokenData[QStringLiteral("accessToken")] = QStringLiteral("new-at");
    tokenData[QStringLiteral("expiresAt")] = newExpires;
    const QString stdOutput = QString::fromUtf8(QJsonDocument(tokenData).toJson(QJsonDocument::Compact));

    provider.persistRefreshedTokens(stdOutput, configKey, &settings);

    const QJsonObject updated = settings.getProviderConfig(QStringLiteral("dropbox"), configKey);
    QCOMPARE(updated.value(QStringLiteral("accessToken")).toString(), QStringLiteral("new-at"));
    QCOMPARE(updated.value(QStringLiteral("expiresAt")).toVariant().toLongLong(), newExpires);
    // CRITICAL: Dropbox refresh response carries no refresh_token. The
    // persisted refreshToken must remain the original -- a regression that
    // overwrote it with empty would silently break long-term auth.
    QCOMPARE(updated.value(QStringLiteral("refreshToken")).toString(), QStringLiteral("rt-must-survive"));
    QCOMPARE(updated.value(QStringLiteral("appKey")).toString(), QStringLiteral("ak-original"));
    QCOMPARE(updated.value(QStringLiteral("remotePath")).toString(), QStringLiteral("/db.kdbx"));
    QCOMPARE(updated.value(QStringLiteral("name")).toString(), configKey);
}

void TestDropboxSyncProvider::testPersistRefreshedTokens_unknownConfigKey_noopWithWarning()
{
    DropboxSyncProvider provider;
    RemoteSettings settings(QSharedPointer<Database>(), nullptr);

    QJsonObject tokenData;
    tokenData[QStringLiteral("accessToken")] = QStringLiteral("new-at");
    const QString stdOutput = QString::fromUtf8(QJsonDocument(tokenData).toJson(QJsonDocument::Compact));

    QTest::ignoreMessage(
        QtWarningMsg,
        "DropboxSyncProvider: no Dropbox config found for 'does-not-exist' to update tokens");
    provider.persistRefreshedTokens(stdOutput, QStringLiteral("does-not-exist"), &settings);

    // Settings unchanged: getProviderConfig for the unknown key still empty.
    QVERIFY(settings.getProviderConfig(QStringLiteral("dropbox"), QStringLiteral("does-not-exist")).isEmpty());
}

void TestDropboxSyncProvider::testPersistRefreshedTokens_malformedJson_noopWithWarning()
{
    DropboxSyncProvider provider;
    RemoteSettings settings(QSharedPointer<Database>(), nullptr);

    const QString configKey = QStringLiteral("k");
    QJsonObject existing;
    existing[QStringLiteral("type")] = QStringLiteral("dropbox");
    existing[QStringLiteral("name")] = configKey;
    existing[QStringLiteral("accessToken")] = QStringLiteral("stay");
    existing[QStringLiteral("refreshToken")] = QStringLiteral("rt");
    settings.setProviderConfig(QStringLiteral("dropbox"), configKey, existing);

    QTest::ignoreMessage(QtWarningMsg, "DropboxSyncProvider: failed to parse refreshed token JSON for persist");
    provider.persistRefreshedTokens(QStringLiteral("{ broken"), configKey, &settings);

    const QJsonObject updated = settings.getProviderConfig(QStringLiteral("dropbox"), configKey);
    QCOMPARE(updated.value(QStringLiteral("accessToken")).toString(), QStringLiteral("stay"));
    QCOMPARE(updated.value(QStringLiteral("refreshToken")).toString(), QStringLiteral("rt"));
}
