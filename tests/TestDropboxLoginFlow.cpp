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

#include "TestDropboxLoginFlow.h"

#include "crypto/Crypto.h"
#include "remotesync/DropboxLoginFlow.h"

#include <QHostAddress>
#include <QRegularExpression>
#include <QSet>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTest>
#include <QUrl>
#include <QUrlQuery>

QTEST_GUILESS_MAIN(TestDropboxLoginFlow)

namespace
{
    // The DropboxLoginFlow implementation hardcodes this port for the localhost
    // callback. To force manual-fallback in tests we bind a blocker QTcpServer
    // to it before driving the flow.
    constexpr quint16 kLocalCallbackPort = 12345;
} // namespace

void TestDropboxLoginFlow::initTestCase()
{
    QVERIFY(Crypto::init());
}

// ---------------------------------------------------------------------------
// PKCE pure statics
// ---------------------------------------------------------------------------

void TestDropboxLoginFlow::testGenerateCodeVerifier_lengthAndCharset()
{
    const QString verifier = DropboxLoginFlow::generateCodeVerifier();
    // RFC 7636 §4.1: code_verifier = high-entropy string, 43..128 chars,
    // unreserved char set [A-Z / a-z / 0-9 / "-" / "." / "_" / "~"]. The
    // base64url-without-padding encoding the impl uses yields chars from
    // [A-Za-z0-9_-], which is a subset.
    QVERIFY2(verifier.length() >= 43 && verifier.length() <= 128,
             qPrintable(QStringLiteral("verifier length out of RFC range: %1").arg(verifier.length())));

    const QRegularExpression base64Url(QStringLiteral("^[A-Za-z0-9_-]+$"));
    QVERIFY2(base64Url.match(verifier).hasMatch(),
             qPrintable(QStringLiteral("verifier contains invalid chars: %1").arg(verifier)));
    // Explicit no-padding / no '+' / no '/' assertions (the production
    // base64url encoding must use Base64UrlEncoding|OmitTrailingEquals).
    QVERIFY(!verifier.contains(QLatin1Char('=')));
    QVERIFY(!verifier.contains(QLatin1Char('+')));
    QVERIFY(!verifier.contains(QLatin1Char('/')));
}

void TestDropboxLoginFlow::testGenerateCodeVerifier_isRandom()
{
    QSet<QString> seen;
    for (int i = 0; i < 10; ++i) {
        seen.insert(DropboxLoginFlow::generateCodeVerifier());
    }
    QCOMPARE(seen.size(), 10);
}

void TestDropboxLoginFlow::testDeriveCodeChallenge_S256_isDeterministic()
{
    const QString a = DropboxLoginFlow::deriveCodeChallenge(QStringLiteral("known-input"));
    const QString b = DropboxLoginFlow::deriveCodeChallenge(QStringLiteral("known-input"));
    QCOMPARE(a, b);
    QVERIFY(!a.isEmpty());
}

void TestDropboxLoginFlow::testDeriveCodeChallenge_S256_matchesRFC7636Vector()
{
    // RFC 7636 Appendix B test vector:
    //   code_verifier  = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk"
    //   code_challenge = "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM"
    const QString verifier = QStringLiteral("dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk");
    const QString expected = QStringLiteral("E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM");
    QCOMPARE(DropboxLoginFlow::deriveCodeChallenge(verifier), expected);
}

void TestDropboxLoginFlow::testDeriveCodeChallenge_differentInputsProduceDifferentOutputs()
{
    const QString a = DropboxLoginFlow::deriveCodeChallenge(QStringLiteral("a"));
    const QString b = DropboxLoginFlow::deriveCodeChallenge(QStringLiteral("b"));
    QVERIFY(a != b);
}

// ---------------------------------------------------------------------------
// State guards on submitManualCode
// ---------------------------------------------------------------------------

void TestDropboxLoginFlow::testSubmitManualCode_withoutManualFallbackState_emitsFailed()
{
    DropboxLoginFlow flow;
    QSignalSpy failedSpy(&flow, &DropboxLoginFlow::authorizationFailed);

    flow.submitManualCode(QStringLiteral("anycode"), 1000);

    QCOMPARE(failedSpy.count(), 1);
}

void TestDropboxLoginFlow::testSubmitManualCode_emptyCode_emitsFailed()
{
    QTcpServer blocker;
    if (!blocker.listen(QHostAddress::LocalHost, kLocalCallbackPort)) {
        QSKIP("Cannot bind localhost:12345 — port unavailable on this machine");
    }

    DropboxLoginFlow flow;
    flow.setBrowserOpener([](const QUrl&) {});
    QSignalSpy fallbackSpy(&flow, &DropboxLoginFlow::authorizationManualFallback);
    flow.startAuthorization(QStringLiteral("appkey"), 1000);
    QCOMPARE(fallbackSpy.count(), 1);

    QSignalSpy failedSpy(&flow, &DropboxLoginFlow::authorizationFailed);
    flow.submitManualCode(QStringLiteral(""), 1000);

    QCOMPARE(failedSpy.count(), 1);
    const QString banner = failedSpy.first().at(0).toString();
    QVERIFY2(banner.contains(QStringLiteral("required"), Qt::CaseInsensitive),
             qPrintable(QStringLiteral("unexpected banner: %1").arg(banner)));
}

void TestDropboxLoginFlow::testSubmitManualCode_whitespaceCode_emitsFailed()
{
    QTcpServer blocker;
    if (!blocker.listen(QHostAddress::LocalHost, kLocalCallbackPort)) {
        QSKIP("Cannot bind localhost:12345 — port unavailable on this machine");
    }

    DropboxLoginFlow flow;
    flow.setBrowserOpener([](const QUrl&) {});
    QSignalSpy fallbackSpy(&flow, &DropboxLoginFlow::authorizationManualFallback);
    flow.startAuthorization(QStringLiteral("appkey"), 1000);
    QCOMPARE(fallbackSpy.count(), 1);

    QSignalSpy failedSpy(&flow, &DropboxLoginFlow::authorizationFailed);
    flow.submitManualCode(QStringLiteral("   \t\n"), 1000);

    QCOMPARE(failedSpy.count(), 1);
}

// ---------------------------------------------------------------------------
// startAuthorization guards
// ---------------------------------------------------------------------------

void TestDropboxLoginFlow::testStartAuthorization_emptyAppKey_emitsFailed()
{
    DropboxLoginFlow flow;
    QSignalSpy failedSpy(&flow, &DropboxLoginFlow::authorizationFailed);
    QSignalSpy completedSpy(&flow, &DropboxLoginFlow::authorizationCompleted);

    flow.startAuthorization(QStringLiteral(""), 1000);

    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(completedSpy.count(), 0);
    const QString banner = failedSpy.first().at(0).toString();
    QVERIFY2(banner.contains(QStringLiteral("App Key"), Qt::CaseInsensitive),
             qPrintable(QStringLiteral("unexpected banner: %1").arg(banner)));
}

void TestDropboxLoginFlow::testStartAuthorization_browserOpenerCalledWithCorrectQuery()
{
    DropboxLoginFlow flow;
    QUrl capturedUrl;
    bool wasCalled = false;
    flow.setBrowserOpener([&capturedUrl, &wasCalled](const QUrl& url) {
        capturedUrl = url;
        wasCalled = true;
    });

    flow.startAuthorization(QStringLiteral("test-app-key"), 1000);

    QVERIFY(wasCalled);

    const QString baseUrl = capturedUrl.toString(QUrl::RemoveQuery);
    QCOMPARE(baseUrl, QStringLiteral("https://www.dropbox.com/oauth2/authorize"));

    const QUrlQuery q(capturedUrl);
    QCOMPARE(q.queryItemValue(QStringLiteral("client_id")), QStringLiteral("test-app-key"));
    QCOMPARE(q.queryItemValue(QStringLiteral("response_type")), QStringLiteral("code"));
    QCOMPARE(q.queryItemValue(QStringLiteral("code_challenge_method")), QStringLiteral("S256"));
    // token_access_type=offline is what makes Dropbox issue a refresh_token; a
    // regression that drops this param silently breaks long-term auth.
    QCOMPARE(q.queryItemValue(QStringLiteral("token_access_type")), QStringLiteral("offline"));

    QVERIFY(q.hasQueryItem(QStringLiteral("code_challenge")));
    QVERIFY(!q.queryItemValue(QStringLiteral("code_challenge")).isEmpty());
    QVERIFY(q.hasQueryItem(QStringLiteral("state")));
    QVERIFY(!q.queryItemValue(QStringLiteral("state")).isEmpty());

    // Whether redirect_uri is present depends on whether the local port could
    // be bound on the test machine — don't assert on it here.

    flow.cancel();
}

void TestDropboxLoginFlow::testStartAuthorization_manualFallback_emitsManualFallbackWithVerifier()
{
    QTcpServer blocker;
    if (!blocker.listen(QHostAddress::LocalHost, kLocalCallbackPort)) {
        QSKIP("Cannot bind localhost:12345 — port unavailable on this machine");
    }

    DropboxLoginFlow flow;
    QUrl capturedUrl;
    flow.setBrowserOpener([&capturedUrl](const QUrl& url) { capturedUrl = url; });

    QSignalSpy fallbackSpy(&flow, &DropboxLoginFlow::authorizationManualFallback);
    flow.startAuthorization(QStringLiteral("appkey"), 1000);

    QCOMPARE(fallbackSpy.count(), 1);
    const QString verifier = fallbackSpy.first().at(0).toString();
    QVERIFY2(verifier.length() >= 43 && verifier.length() <= 128,
             qPrintable(QStringLiteral("verifier length out of RFC range: %1").arg(verifier.length())));
    const QRegularExpression base64Url(QStringLiteral("^[A-Za-z0-9_-]+$"));
    QVERIFY(base64Url.match(verifier).hasMatch());

    // Manual fallback branch must NOT include redirect_uri in the authorize URL
    // — Dropbox's PKCE token exchange would otherwise demand a matching value.
    const QUrlQuery q(capturedUrl);
    QVERIFY(!q.hasQueryItem(QStringLiteral("redirect_uri")));
}

// ---------------------------------------------------------------------------
// cancel semantics
// ---------------------------------------------------------------------------

void TestDropboxLoginFlow::testCancel_inIdle_isNoop()
{
    DropboxLoginFlow flow;
    QSignalSpy cancelledSpy(&flow, &DropboxLoginFlow::authorizationCancelled);
    QSignalSpy failedSpy(&flow, &DropboxLoginFlow::authorizationFailed);
    QSignalSpy completedSpy(&flow, &DropboxLoginFlow::authorizationCompleted);

    flow.cancel();

    QCOMPARE(cancelledSpy.count(), 0);
    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(completedSpy.count(), 0);
}

void TestDropboxLoginFlow::testCancel_inAuthorizing_emitsAuthorizationCancelled()
{
    DropboxLoginFlow flow;
    flow.setBrowserOpener([](const QUrl&) {});
    // Bypass manual-fallback by ensuring the port is bindable: if it's NOT
    // bindable, the flow lands in ManualFallback rather than Authorizing.
    // Probe with a temporary listener; if probe succeeds, port is free and the
    // real start will succeed too (we close the probe first).
    QTcpServer probe;
    if (!probe.listen(QHostAddress::LocalHost, kLocalCallbackPort)) {
        QSKIP("Cannot bind localhost:12345 — port unavailable, cannot reach Authorizing state");
    }
    probe.close();

    QSignalSpy cancelledSpy(&flow, &DropboxLoginFlow::authorizationCancelled);
    QSignalSpy failedSpy(&flow, &DropboxLoginFlow::authorizationFailed);
    QSignalSpy completedSpy(&flow, &DropboxLoginFlow::authorizationCompleted);

    // Long timeout so the timer doesn't preempt our cancel.
    flow.startAuthorization(QStringLiteral("appkey"), 60000);
    flow.cancel();

    QCOMPARE(cancelledSpy.count(), 1);
    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(completedSpy.count(), 0);
}

void TestDropboxLoginFlow::testCancel_inManualFallback_emitsAuthorizationCancelled()
{
    QTcpServer blocker;
    if (!blocker.listen(QHostAddress::LocalHost, kLocalCallbackPort)) {
        QSKIP("Cannot bind localhost:12345 — port unavailable on this machine");
    }

    DropboxLoginFlow flow;
    flow.setBrowserOpener([](const QUrl&) {});
    QSignalSpy fallbackSpy(&flow, &DropboxLoginFlow::authorizationManualFallback);
    flow.startAuthorization(QStringLiteral("appkey"), 1000);
    QCOMPARE(fallbackSpy.count(), 1);

    QSignalSpy cancelledSpy(&flow, &DropboxLoginFlow::authorizationCancelled);
    flow.cancel();

    QCOMPARE(cancelledSpy.count(), 1);
}

void TestDropboxLoginFlow::testCancel_afterTerminalCompletedOrFailed_doesNotReEmit()
{
    DropboxLoginFlow flow;
    QSignalSpy failedSpy(&flow, &DropboxLoginFlow::authorizationFailed);
    // Drive to Failed via empty-appKey.
    flow.startAuthorization(QStringLiteral(""), 1000);
    QCOMPARE(failedSpy.count(), 1);

    QSignalSpy cancelledSpy(&flow, &DropboxLoginFlow::authorizationCancelled);
    flow.cancel();

    QCOMPARE(cancelledSpy.count(), 0);
}
