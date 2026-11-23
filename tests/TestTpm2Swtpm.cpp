/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#include "TestTpm2Swtpm.h"

#include "config-keepassx.h"
#include "core/Config.h"
#include "core/Database.h"
#include "crypto/Crypto.h"
#include "gui/DatabaseOpenWidget.h"
#include "keys/ChallengeResponseKey.h"
#include "keys/CompositeKey.h"
#include "keys/PasswordKey.h"
#include "keys/drivers/YubiKeyInterfaceTPM.h"

#include <QFile>

#include <atomic>
#include <QMessageAuthenticationCode>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

QTEST_GUILESS_MAIN(TestTpm2Swtpm)

namespace
{
    constexpr uint HANDLE_PLAIN = 0x81010004; // no authValue
    constexpr uint HANDLE_PIN = 0x81010005; // authValue "1234", counted by dictionary-attack protection
    constexpr uint HANDLE_PIN_NODA = 0x81010006; // authValue "1234", exempt from lockout
    constexpr uint HANDLE_RSA = 0x81010007; // not an HMAC key, must never be listed
    constexpr uint HANDLE_PLAIN_NODA = 0x81010008; // no authValue, NODA
    constexpr uint HANDLE_POLICY = 0x81010009; // HMAC key bound to an authorization policy (not supported)
    constexpr uint HANDLE_POLICY_OR = 0x8101000A; // policy and userWithAuth: still bound to a policy
    constexpr uint HANDLE_NO_USERAUTH = 0x8101000B; // HMAC key without userWithAuth (cannot use a password)

    constexpr int FIXED = YubiKey::TPM_SLOT_FLAG | YubiKey::TPM_MODE_FIXED64;
    constexpr int VARIABLE = YubiKey::TPM_SLOT_FLAG | YubiKey::TPM_MODE_VARIABLE;
    constexpr int PIN = YubiKey::TPM_PIN_FLAG;

    Botan::secure_vector<char> secure(const char* text)
    {
        return Botan::secure_vector<char>(text, text + strlen(text));
    }

    /**
     * Independent reference for what a YubiKey computes, deliberately not sharing code with the driver:
     * PKCS7-style padding to 64 bytes, then (HMAC_LT64) removal of the trailing run of the final byte.
     */
    QByteArray referenceResponse(const QByteArray& secret, QByteArray challenge, bool variableLength)
    {
        const int pad = 64 - challenge.size();
        if (pad > 0) {
            challenge.append(QByteArray(pad, static_cast<char>(pad)));
        }
        if (variableLength) {
            const char last = challenge.back();
            while (!challenge.isEmpty() && challenge.back() == last) {
                challenge.chop(1);
            }
        }
        return QMessageAuthenticationCode::hash(challenge, secret, QCryptographicHash::Sha1);
    }

    QByteArray toBytes(const Botan::secure_vector<char>& v)
    {
        return QByteArray(v.data(), static_cast<qsizetype>(v.size()));
    }
} // namespace

bool TestTpm2Swtpm::run(const QString& program, const QStringList& args)
{
    QProcess p;
    p.setWorkingDirectory(m_dir->path());
    p.start(program, args);
    if (!p.waitForFinished(30000) || p.exitCode() != 0) {
        qWarning() << program << args << "failed:" << p.readAllStandardError() << p.readAllStandardOutput();
        return false;
    }
    return true;
}

// The software TPM has no resource manager, so transient objects must be flushed after each step
bool TestTpm2Swtpm::provision(uint handle, const QString& attributes, const QString& auth, const QString& policyFile)
{
    const auto hex = QStringLiteral("0x%1").arg(handle, 8, 16, QLatin1Char('0'));
    QStringList import = {
        "-C", "primary.ctx", "-G", "hmac:sha1", "-i", "secret.bin", "-u", "k.pub", "-r", "k.priv", "-a", attributes};
    if (!auth.isEmpty()) {
        import << "-p" << auth;
    }
    if (!policyFile.isEmpty()) {
        import << "-L" << policyFile;
    }
    return run("tpm2_createprimary", {"-C", "o", "-c", "primary.ctx"}) && run("tpm2_flushcontext", {"-t"})
           && run("tpm2_import", import) && run("tpm2_flushcontext", {"-t"})
           && run("tpm2_load", {"-C", "primary.ctx", "-u", "k.pub", "-r", "k.priv", "-c", "k.ctx"})
           && run("tpm2_flushcontext", {"-t"}) && run("tpm2_evictcontrol", {"-C", "o", "-c", "k.ctx", hex})
           && run("tpm2_flushcontext", {"-t"});
}

// Current value of the TPM's failed-authorization counter, or -1 if it cannot be read
int TestTpm2Swtpm::lockoutCounter()
{
    QProcess p;
    p.setWorkingDirectory(m_dir->path());
    p.start("tpm2_getcap", {"properties-variable"});
    if (!p.waitForFinished(30000) || p.exitCode() != 0) {
        return -1;
    }
    const auto match = QRegularExpression(R"(TPM2_PT_LOCKOUT_COUNTER:\s+(0x[0-9a-fA-F]+))")
                           .match(QString::fromUtf8(p.readAllStandardOutput()));
    return match.hasMatch() ? match.captured(1).toInt(nullptr, 16) : -1;
}

// The software TPM has an empty lockout authorization: forget all recorded failed attempts
bool TestTpm2Swtpm::clearLockout()
{
    return run("tpm2_dictionarylockout", {"--clear-lockout"});
}

bool TestTpm2Swtpm::provisionRsa(uint handle)
{
    const auto hex = QStringLiteral("0x%1").arg(handle, 8, 16, QLatin1Char('0'));
    return run("tpm2_createprimary", {"-C", "o", "-c", "primary.ctx"}) && run("tpm2_flushcontext", {"-t"})
           && run("tpm2_create", {"-C", "primary.ctx", "-G", "rsa", "-c", "r.ctx"}) && run("tpm2_flushcontext", {"-t"})
           && run("tpm2_evictcontrol", {"-C", "o", "-c", "r.ctx", hex}) && run("tpm2_flushcontext", {"-t"});
}

void TestTpm2Swtpm::initTestCase()
{
#ifndef KPXC_FEATURE_TPM2
    QSKIP("TPM2 support is not compiled in.");
#else
    for (const char* tool : {"swtpm",
                             "tpm2_getcap",
                             "tpm2_createprimary",
                             "tpm2_import",
                             "tpm2_load",
                             "tpm2_create",
                             "tpm2_createpolicy",
                             "tpm2_evictcontrol",
                             "tpm2_flushcontext",
                             "tpm2_dictionarylockout"}) {
        if (QStandardPaths::findExecutable(tool).isEmpty()) {
            QSKIP(qPrintable(QStringLiteral("%1 is not installed.").arg(tool)));
        }
    }

    m_dir.reset(new QTemporaryDir());
    QVERIFY(m_dir->isValid());

    // The swtpm TCTI expects the control channel on the port right after the data port, so find a free pair
    quint16 dataPort = 0;
    for (int attempt = 0; attempt < 50 && dataPort == 0; ++attempt) {
        QTcpServer a;
        QTcpServer b;
        if (a.listen(QHostAddress::LocalHost, 0) && a.serverPort() < 65535
            && b.listen(QHostAddress::LocalHost, a.serverPort() + 1)) {
            dataPort = a.serverPort();
        }
    }
    QVERIFY2(dataPort != 0, "no free pair of adjacent TCP ports");
    const quint16 ctrlPort = dataPort + 1;

    QVERIFY(QDir(m_dir->filePath("state")).mkpath("."));
    m_swtpm.start("swtpm",
                  {"socket",
                   "--tpm2",
                   "--tpmstate",
                   "dir=" + m_dir->filePath("state"),
                   "--server",
                   QStringLiteral("type=tcp,port=%1").arg(dataPort),
                   "--ctrl",
                   QStringLiteral("type=tcp,port=%1").arg(ctrlPort),
                   "--flags",
                   "startup-clear"});
    QVERIFY(m_swtpm.waitForStarted(5000));

    // Wait until the data port accepts connections
    bool up = false;
    for (int i = 0; i < 50 && !up; ++i) {
        QTcpSocket s;
        s.connectToHost(QHostAddress::LocalHost, dataPort);
        up = s.waitForConnected(200);
        if (!up) {
            QTest::qWait(100);
        }
    }
    QVERIFY2(up, "swtpm did not start");

    m_tcti = QStringLiteral("swtpm:port=%1").arg(dataPort);
    // Honoured by both tpm2-tools and the KeePassXC driver
    qputenv("TPM2TOOLS_TCTI", m_tcti.toUtf8());

    // SAFETY: this test provisions keys and deliberately sends wrong PINs until the TPM locks out. Never go on
    // unless the connection really is the software TPM started above, never a hardware TPM.
    {
        QProcess probe;
        probe.start("tpm2_getcap", {"properties-fixed"});
        probe.waitForFinished(15000);
        const auto probeError = probe.readAllStandardError();
        // The TCTI that talks to swtpm is a separate library on some distributions (e.g. libtss2-tcti-swtpm0),
        // so tpm2-tools and swtpm can both be installed without it. Skip instead of failing in that case.
        if (probe.exitCode() != 0 && probeError.contains("tcti")) {
            m_swtpm.terminate();
            m_swtpm.waitForFinished(3000);
            QSKIP("The swtpm TCTI library (libtss2-tcti-swtpm) is not installed.");
        }
        QVERIFY2(probe.exitCode() == 0, probeError.constData());

        // swtpm/libtpms identify as manufacturer "IBM" with vendor string "SW" (software TPM)
        const auto info = QString::fromUtf8(probe.readAllStandardOutput());
        QVERIFY2(QRegularExpression(R"(TPM2_PT_MANUFACTURER:\s+raw:\s+0x\w+\s+value:\s+"IBM")").match(info).hasMatch()
                     && QRegularExpression(R"(TPM2_PT_VENDOR_STRING_1:\s+raw:\s+0x\w+\s+value:\s+"SW")")
                            .match(info)
                            .hasMatch(),
                 "Refusing to run: the connected TPM does not identify as a software TPM");
    }

    QVERIFY(Crypto::init());

    // Settings for the application level tests below: temporary files only, TPM2 keys enabled
    QVERIFY(m_configFile.open());
    QVERIFY(m_localConfigFile.open());
    Config::createConfigFromFile(m_configFile.fileName(), m_localConfigFile.fileName());
    config()->set(Config::Security_EnableTpm2, true);

    m_secret = QByteArray::fromHex("1ce30fd78d20dcfa40b50c18779afb0f02288db7");
    QFile secretFile(m_dir->filePath("secret.bin"));
    QVERIFY(secretFile.open(QIODevice::WriteOnly));
    secretFile.write(m_secret);
    secretFile.close();

    QVERIFY(provision(HANDLE_PLAIN, "sign|userwithauth", {}));
    QVERIFY(provision(HANDLE_PIN, "sign|userwithauth", "1234"));
    QVERIFY(provision(HANDLE_PIN_NODA, "sign|userwithauth|noda", "1234"));
    QVERIFY(provision(HANDLE_PLAIN_NODA, "sign|userwithauth|noda", {}));
    QVERIFY(provisionRsa(HANDLE_RSA));

    // Keys that cannot be used by KeePassXC: bound to an authorization policy (here PCR values), or without
    // userWithAuth
    QVERIFY(run("tpm2_createpolicy", {"--policy-pcr", "-l", "sha256:0,7", "-L", "pcr.policy"}));
    QVERIFY(run("tpm2_flushcontext", {"-t"}));
    QVERIFY(provision(HANDLE_POLICY, "sign", "1234", "pcr.policy"));
    QVERIFY(provision(HANDLE_POLICY_OR, "sign|userwithauth", "1234", "pcr.policy"));
    QVERIFY(provision(HANDLE_NO_USERAUTH, "sign", {}));
#endif
}

void TestTpm2Swtpm::init()
{
    // Every test must still be talking to the software TPM started by this test
    QCOMPARE(qEnvironmentVariable("TPM2TOOLS_TCTI"), m_tcti);
    QVERIFY(m_tcti.startsWith("swtpm:port="));
}

void TestTpm2Swtpm::cleanup()
{
    YubiKey::setPinProvider({});
}

void TestTpm2Swtpm::cleanupTestCase()
{
    qunsetenv("TPM2TOOLS_TCTI");
    if (m_swtpm.state() != QProcess::NotRunning) {
        m_swtpm.terminate();
        if (!m_swtpm.waitForFinished(3000)) {
            m_swtpm.kill();
            m_swtpm.waitForFinished(3000);
        }
    }
}

void TestTpm2Swtpm::testListKeys()
{
    int connected = 0;
    const auto keys = YubiKeyInterfaceTPM::instance()->findValidKeys(connected);

    auto has = [&](uint handle, int slot) { return keys.contains({handle, slot}); };

    // The TPM does not reveal whether a key has an authValue, so keys that count failures are offered both ways
    for (auto handle : {HANDLE_PLAIN, HANDLE_PIN}) {
        QVERIFY(has(handle, FIXED));
        QVERIFY(has(handle, VARIABLE));
        QVERIFY(has(handle, FIXED | PIN));
        QVERIFY(has(handle, VARIABLE | PIN));
    }

    // NODA keys can be probed safely, so only the matching variant is offered
    QVERIFY(has(HANDLE_PLAIN_NODA, FIXED));
    QVERIFY(has(HANDLE_PLAIN_NODA, VARIABLE));
    QVERIFY(!has(HANDLE_PLAIN_NODA, FIXED | PIN));
    QVERIFY(!has(HANDLE_PIN_NODA, FIXED));
    QVERIFY(!has(HANDLE_PIN_NODA, VARIABLE));
    QVERIFY(has(HANDLE_PIN_NODA, FIXED | PIN));
    QVERIFY(has(HANDLE_PIN_NODA, VARIABLE | PIN));

    // Not an HMAC key
    QVERIFY(!has(HANDLE_RSA, FIXED));
    QVERIFY(!has(HANDLE_RSA, FIXED | PIN));

    // HMAC keys that cannot be used are not offered
    for (auto handle : {HANDLE_POLICY, HANDLE_POLICY_OR, HANDLE_NO_USERAUTH}) {
        QVERIFY(!has(handle, FIXED));
        QVERIFY(!has(handle, FIXED | PIN));
    }

    QCOMPARE(connected, 4);

    // Labels name the handle and the mode
    QVERIFY(keys.value({HANDLE_PLAIN, FIXED}).contains("0x81010004"));
    QVERIFY(keys.value({HANDLE_PLAIN, FIXED}).contains("Fixed 64-byte"));
    QVERIFY(keys.value({HANDLE_PLAIN, VARIABLE}).contains("HMAC_LT64"));
    QVERIFY(keys.value({HANDLE_PIN, FIXED | PIN}).contains("PIN/password"));
}

void TestTpm2Swtpm::testResponseMatchesReference_data()
{
    QTest::addColumn<QByteArray>("challenge");
    QTest::addColumn<bool>("variable");

    QByteArray seq(32, 0);
    for (int i = 0; i < 32; ++i) {
        seq[i] = static_cast<char>(i);
    }
    // 32 byte database master seed that happens to end in the padding byte (0x20)
    QByteArray endsInPad = seq;
    endsInPad[31] = 0x20;

    for (bool variable : {false, true}) {
        const char* mode = variable ? "variable" : "fixed";
        QTest::addRow("%s-32-bytes", mode) << seq << variable;
        QTest::addRow("%s-ends-in-pad-byte", mode) << endsInPad << variable;
        QTest::addRow("%s-full-64-bytes", mode) << (seq + seq) << variable;
        QTest::addRow("%s-short", mode) << QByteArray("abc") << variable;
        QTest::addRow("%s-empty", mode) << QByteArray() << variable;
    }
}

void TestTpm2Swtpm::testResponseMatchesReference()
{
    QFETCH(QByteArray, challenge);
    QFETCH(bool, variable);

    Botan::secure_vector<char> response;
    const auto result = YubiKeyInterfaceTPM::instance()->challengeWithPin(
        {HANDLE_PLAIN, variable ? VARIABLE : FIXED}, challenge, response, nullptr);
    QCOMPARE(result, YubiKey::ChallengeResult::YCR_SUCCESS);
    QCOMPARE(toBytes(response), referenceResponse(m_secret, challenge, variable));
}

void TestTpm2Swtpm::testPinKeyGivesSameResponse()
{
    // Same secret, protected by a PIN: identical response once the PIN is supplied
    const QByteArray challenge(32, 'x');
    for (int mode : {FIXED, VARIABLE}) {
        Botan::secure_vector<char> plain;
        Botan::secure_vector<char> withPin;
        auto pin = secure("1234");
        QCOMPARE(YubiKeyInterfaceTPM::instance()->challengeWithPin({HANDLE_PLAIN, mode}, challenge, plain, nullptr),
                 YubiKey::ChallengeResult::YCR_SUCCESS);
        QCOMPARE(YubiKeyInterfaceTPM::instance()->challengeWithPin(
                     {HANDLE_PIN_NODA, mode | PIN}, challenge, withPin, &pin),
                 YubiKey::ChallengeResult::YCR_SUCCESS);
        QCOMPARE(toBytes(withPin), toBytes(plain));
    }

    // The test-challenge helper reports the result code as well
    auto pin = secure("1234");
    YubiKey::ChallengeResult result = YubiKey::ChallengeResult::YCR_ERROR;
    QVERIFY(YubiKeyInterfaceTPM::instance()->testChallengeWithPin(
        {HANDLE_PIN_NODA, FIXED | PIN}, nullptr, &pin, &result));
    QCOMPARE(result, YubiKey::ChallengeResult::YCR_SUCCESS);
}

void TestTpm2Swtpm::testMissingHandle()
{
    Botan::secure_vector<char> response;
    QCOMPARE(
        YubiKeyInterfaceTPM::instance()->challengeWithPin({0x81010099, FIXED}, QByteArray(32, 0), response, nullptr),
        YubiKey::ChallengeResult::YCR_KEYNOTFOUND);
    QVERIFY(response.empty());

    // A handle that exists but is not an HMAC key is treated as not found too
    QCOMPARE(
        YubiKeyInterfaceTPM::instance()->challengeWithPin({HANDLE_RSA, FIXED}, QByteArray(32, 0), response, nullptr),
        YubiKey::ChallengeResult::YCR_KEYNOTFOUND);
}

void TestTpm2Swtpm::testPinSlotWithoutPinCostsNoAttempt()
{
    // Without a PIN the driver must not send an (empty) password to a key that needs one: the TPM would count
    // that as a failed attempt towards its lockout protection
    auto* tpm = YubiKeyInterfaceTPM::instance();
    const int before = lockoutCounter();
    QVERIFY(before >= 0);

    Botan::secure_vector<char> response;
    QCOMPARE(tpm->challengeWithPin({HANDLE_PIN, FIXED | PIN}, QByteArray(32, 'x'), response, nullptr),
             YubiKey::ChallengeResult::YCR_ERROR);
    QVERIFY2(tpm->errorMessage().contains("required"), qPrintable(tpm->errorMessage()));
    Botan::secure_vector<char> emptyPin;
    QCOMPARE(tpm->challengeWithPin({HANDLE_PIN, FIXED | PIN}, QByteArray(32, 'x'), response, &emptyPin),
             YubiKey::ChallengeResult::YCR_ERROR);

    QCOMPARE(lockoutCounter(), before);
}

void TestTpm2Swtpm::testUnusableKeysAreExplained()
{
    auto* tpm = YubiKeyInterfaceTPM::instance();
    const int before = lockoutCounter();
    QVERIFY(before >= 0);

    // The key list explains why TPM2 keys that exist cannot be used
    int connected = 0;
    tpm->findValidKeys(connected);
    const auto hint = tpm->unusableKeysHint();
    QVERIFY2(hint.contains("authorization policy"), qPrintable(hint));
    QVERIFY2(hint.contains("PCR"), qPrintable(hint));
    QVERIFY2(hint.contains("userWithAuth"), qPrintable(hint));

    // Using such a key says what is wrong with it, with and without a PIN
    Botan::secure_vector<char> response;
    auto pin = secure("1234");
    struct Case
    {
        uint handle;
        const char* expected;
    };
    for (const auto& c : {Case{HANDLE_POLICY, "authorization policy"},
                          Case{HANDLE_POLICY_OR, "authorization policy"},
                          Case{HANDLE_NO_USERAUTH, "userWithAuth"},
                          Case{HANDLE_RSA, "not an HMAC-SHA1"},
                          Case{0x81010099, "No TPM2 key found"}}) {
        for (bool withPin : {false, true}) {
            const int slot = withPin ? (FIXED | PIN) : FIXED;
            QCOMPARE(tpm->challengeWithPin({c.handle, slot}, QByteArray(32, 'x'), response, withPin ? &pin : nullptr),
                     YubiKey::ChallengeResult::YCR_KEYNOTFOUND);
            QVERIFY2(tpm->errorMessage().contains(c.expected),
                     qPrintable(QStringLiteral("0x%1: %2").arg(c.handle, 0, 16).arg(tpm->errorMessage())));
            QVERIFY(tpm->errorMessage().contains(QStringLiteral("%1").arg(c.handle, 8, 16, QLatin1Char('0'))));
        }
    }

    // A policy-bound key is turned away before any authorization is sent: no failed attempt is recorded
    QCOMPARE(lockoutCounter(), before);
}

void TestTpm2Swtpm::testPinNoticeCleanTpm()
{
    // Nothing recorded yet: no warning before prompting
    QVERIFY(YubiKeyInterfaceTPM::instance()->pinNotice({HANDLE_PIN, FIXED | PIN}).isEmpty());
}

void TestTpm2Swtpm::testWrongPinCounted()
{
    auto* tpm = YubiKeyInterfaceTPM::instance();
    auto wrong = secure("0000");
    Botan::secure_vector<char> response;

    QCOMPARE(tpm->challengeWithPin({HANDLE_PIN, FIXED | PIN}, QByteArray(32, 'x'), response, &wrong),
             YubiKey::ChallengeResult::YCR_AUTHFAILED);
    QVERIFY(response.empty());
    // The user is told the TPM counted the attempt and how many remain
    QVERIFY2(tpm->errorMessage().contains("counted this attempt"), qPrintable(tpm->errorMessage()));
    QVERIFY2(tpm->errorMessage().contains("1 of "), qPrintable(tpm->errorMessage()));

    // The next prompt carries a warning about the recorded failure
    QVERIFY(!tpm->pinNotice({HANDLE_PIN, FIXED | PIN}).isEmpty());

    // A key that is exempt from lockout never gets a lockout warning
    QVERIFY(tpm->pinNotice({HANDLE_PIN_NODA, FIXED | PIN}).isEmpty());

    // Supplying no PIN at all for a protected key is refused up front (and is not sent to the TPM)
    QCOMPARE(tpm->challengeWithPin({HANDLE_PIN_NODA, FIXED | PIN}, QByteArray(32, 'x'), response, nullptr),
             YubiKey::ChallengeResult::YCR_ERROR);
}

void TestTpm2Swtpm::testWrongPinNoDa()
{
    auto* tpm = YubiKeyInterfaceTPM::instance();
    auto wrong = secure("0000");
    Botan::secure_vector<char> response;

    QCOMPARE(tpm->challengeWithPin({HANDLE_PIN_NODA, FIXED | PIN}, QByteArray(32, 'x'), response, &wrong),
             YubiKey::ChallengeResult::YCR_AUTHFAILED);
    QVERIFY2(tpm->errorMessage().contains("exempt"), qPrintable(tpm->errorMessage()));
    QVERIFY(!tpm->errorMessage().contains("counted this attempt"));
}

void TestTpm2Swtpm::testPlainEntryOnPinKeyGivesHint()
{
    // Picking the plain entry for a key that has a PIN: the user is told to choose the PIN entry
    auto* tpm = YubiKeyInterfaceTPM::instance();
    Botan::secure_vector<char> response;

    QCOMPARE(tpm->challengeWithPin({HANDLE_PIN, FIXED}, QByteArray(32, 'x'), response, nullptr),
             YubiKey::ChallengeResult::YCR_AUTHFAILED);
    QVERIFY2(tpm->errorMessage().contains("empty password"), qPrintable(tpm->errorMessage()));
    QVERIFY2(tpm->errorMessage().contains("\"PIN/password\""), qPrintable(tpm->errorMessage()));
    QVERIFY2(tpm->errorMessage().contains("counted this attempt"), qPrintable(tpm->errorMessage()));

    // Same for a key that is exempt from lockout
    QCOMPARE(tpm->challengeWithPin({HANDLE_PIN_NODA, FIXED}, QByteArray(32, 'x'), response, nullptr),
             YubiKey::ChallengeResult::YCR_AUTHFAILED);
    QVERIFY2(tpm->errorMessage().contains("\"PIN/password\""), qPrintable(tpm->errorMessage()));
    QVERIFY2(tpm->errorMessage().contains("exempt"), qPrintable(tpm->errorMessage()));
    QVERIFY(!tpm->errorMessage().contains("counted this attempt"));

    // A wrong PIN on the PIN entry does not get that hint
    auto wrong = Botan::secure_vector<char>{'0', '0'};
    QCOMPARE(tpm->challengeWithPin({HANDLE_PIN_NODA, FIXED | PIN}, QByteArray(32, 'x'), response, &wrong),
             YubiKey::ChallengeResult::YCR_AUTHFAILED);
    QVERIFY(!tpm->errorMessage().contains("empty password"));
}

void TestTpm2Swtpm::testLockout()
{
    // Runs last: it locks the TPM for every key that counts failures
    auto* tpm = YubiKeyInterfaceTPM::instance();
    auto wrong = secure("0000");
    Botan::secure_vector<char> response;

    bool lockedOut = false;
    for (int attempt = 0; attempt < 10 && !lockedOut; ++attempt) {
        QCOMPARE(tpm->challengeWithPin({HANDLE_PIN, FIXED | PIN}, QByteArray(32, 'x'), response, &wrong),
                 YubiKey::ChallengeResult::YCR_AUTHFAILED);
        lockedOut = tpm->errorMessage().contains("locked out");
        if (!lockedOut) {
            QVERIFY2(tpm->errorMessage().contains("counted this attempt"), qPrintable(tpm->errorMessage()));
        }
    }
    QVERIFY2(lockedOut, "the TPM never reported lockout");

    // Even the correct PIN is now refused, and this is reported as a lockout (not as a wrong PIN)
    auto right = secure("1234");
    QCOMPARE(tpm->challengeWithPin({HANDLE_PIN, FIXED | PIN}, QByteArray(32, 'x'), response, &right),
             YubiKey::ChallengeResult::YCR_ERROR);
    QVERIFY2(tpm->errorMessage().contains("locked out"), qPrintable(tpm->errorMessage()));

    // Prompts warn about it before the user types anything
    QVERIFY(tpm->pinNotice({HANDLE_PIN, FIXED | PIN}).contains("locked out"));
}

void TestTpm2Swtpm::testPlainEntryOnPinKeyIsTriedOnce()
{
    // Regression test: choosing the plain entry for a key that has a PIN must be refused ONCE. It used to be
    // retried without any prompt until the TPM locked out. Run through ChallengeResponseKey, i.e. the code path
    // used when opening and saving a database.
    QVERIFY(clearLockout());
    const int before = lockoutCounter();
    QCOMPARE(before, 0);

    int prompts = 0;
    YubiKey::setPinProvider([&](const YubiKeySlot&, const QString&, Botan::secure_vector<char>&, bool&) {
        ++prompts;
        return false;
    });
    std::atomic<int> attempts{0};
    QObject context;
    connect(
        YubiKeyInterfaceTPM::instance(), &YubiKeyInterface::challengeStarted, &context, [&] { ++attempts; },
        Qt::DirectConnection);

    ChallengeResponseKey key({HANDLE_PIN, FIXED}); // plain entry, but the key has a PIN
    QVERIFY(!key.challenge(QByteArray(32, 'x')));

    QCOMPARE(attempts.load(), 1);
    QCOMPARE(prompts, 0);
    QCOMPARE(lockoutCounter(), before + 1);
    QVERIFY2(key.error().contains("\"PIN/password\""), qPrintable(key.error()));
    QVERIFY2(!key.error().contains("locked out"), qPrintable(key.error()));
    QVERIFY(clearLockout());
}

void TestTpm2Swtpm::testPinEntryRetriesUntilCancelled()
{
    // With the PIN entry every new attempt follows a new prompt that shows what the TPM said. Uses the key that
    // is exempt from lockout so that the test does not lock the TPM.
    int prompts = 0;
    QStringList messages;
    YubiKey::setPinProvider(
        [&](const YubiKeySlot&, const QString& message, Botan::secure_vector<char>& pin, bool& remember) {
            messages << message;
            if (++prompts > 2) {
                return false; // the user gives up
            }
            pin = secure("0000");
            remember = true;
            return true;
        });
    std::atomic<int> attempts{0};
    QObject context;
    connect(
        YubiKeyInterfaceTPM::instance(), &YubiKeyInterface::challengeStarted, &context, [&] { ++attempts; },
        Qt::DirectConnection);

    ChallengeResponseKey key({HANDLE_PIN_NODA, FIXED | PIN});
    QVERIFY(!key.challenge(QByteArray(32, 'x')));

    QCOMPARE(prompts, 3);
    QCOMPARE(attempts.load(), 2); // one attempt per PIN that was entered, none after cancelling
    QVERIFY(messages.at(0).isEmpty());
    QVERIFY2(messages.at(1).contains("Incorrect"), qPrintable(messages.at(1)));
    QVERIFY2(messages.at(2).contains("Incorrect"), qPrintable(messages.at(2)));
    QVERIFY2(key.error().contains("cancelled"), qPrintable(key.error()));
    QVERIFY(!key.hasCachedPin()); // a wrong PIN is never remembered
}

void TestTpm2Swtpm::testPinCachedAcrossChallenges()
{
    int prompts = 0;
    YubiKey::setPinProvider([&](const YubiKeySlot&, const QString&, Botan::secure_vector<char>& pin, bool& remember) {
        ++prompts;
        pin = secure("1234");
        remember = true;
        return true;
    });

    ChallengeResponseKey key({HANDLE_PIN_NODA, VARIABLE | PIN});
    QVERIFY2(key.challenge(QByteArray(32, 'x')), qPrintable(key.error()));
    QCOMPARE(prompts, 1);
    QCOMPARE(key.rawKey(), referenceResponse(m_secret, QByteArray(32, 'x'), true));
    QVERIFY(key.hasCachedPin());

    // The remembered PIN is used for the next challenge (e.g. the next save), without asking again
    QVERIFY2(key.challenge(QByteArray(32, 'y')), qPrintable(key.error()));
    QCOMPARE(prompts, 1);
    QCOMPARE(key.rawKey(), referenceResponse(m_secret, QByteArray(32, 'y'), true));

    // Locking the database wipes it
    key.clearCachedPin();
    QVERIFY(!key.hasCachedPin());
}

void TestTpm2Swtpm::testQuickUnlockRestoresRememberedPin()
{
    // The Quick Unlock round trip as DatabaseOpenWidget performs it, with a real database: unlock (the PIN is
    // asked for and remembered), serialize the key, restore it from that data, unlock again without any prompt.
    config()->set(Config::Security_QuickUnlock, true);
    config()->set(Config::Security_QuickUnlockTpm2Pin, true);
    const YubiKeySlot slot{HANDLE_PIN_NODA, FIXED | PIN};

    int prompts = 0;
    bool remember = true;
    YubiKey::setPinProvider(
        [&](const YubiKeySlot&, const QString&, Botan::secure_vector<char>& pin, bool& rememberOut) {
            ++prompts;
            pin = secure("1234");
            rememberOut = remember;
            return true;
        });

    const QString file = m_dir->filePath("quickunlock.kdbx");
    {
        auto creation = QSharedPointer<CompositeKey>::create();
        creation->addKey(QSharedPointer<PasswordKey>::create("password"));
        creation->addChallengeResponseKey(QSharedPointer<ChallengeResponseKey>::create(slot));
        Database db;
        QVERIFY(db.setKey(creation));
        QString error;
        QVERIFY2(db.saveAs(file, Database::Atomic, QString(), &error), qPrintable(error));
    }

    // 1. First unlock with the password and the TPM key
    prompts = 0;
    auto first = QSharedPointer<CompositeKey>::create();
    first->addKey(QSharedPointer<PasswordKey>::create("password"));
    auto firstCr = QSharedPointer<ChallengeResponseKey>::create(slot);
    first->addChallengeResponseKey(firstCr);
    {
        Database db;
        QString error;
        QVERIFY2(db.open(file, first, &error), qPrintable(error));
    }
    QCOMPARE(prompts, 1);
    QVERIFY(firstCr->hasCachedPin()); // remembered because the user chose to

    // 2. What Quick Unlock stores, and what it hands back later
    const auto stored = first->serialize();
    auto restored = QSharedPointer<CompositeKey>::create();
    restored->setRawKey(stored);
    QCOMPARE(restored->challengeResponseKeys().size(), 1);
    QVERIFY(restored->challengeResponseKeys().first()->hasCachedPin());

    // 3. Unlock again with the restored key: no prompt
    {
        Database db;
        QString error;
        QVERIFY2(db.open(file, restored, &error), qPrintable(error));
    }
    QCOMPARE(prompts, 1);

    // Without the setting the PIN does not go into the stored data
    config()->set(Config::Security_QuickUnlockTpm2Pin, false);
    auto withoutSetting = QSharedPointer<CompositeKey>::create();
    withoutSetting->setRawKey(first->serialize());
    QVERIFY(!withoutSetting->challengeResponseKeys().first()->hasCachedPin());
    config()->set(Config::Security_QuickUnlockTpm2Pin, true);

    // If the user did not tick "remember" in the dialog, there is nothing to store: the setting alone does not
    // remember a PIN
    remember = false;
    prompts = 0;
    auto noRemember = QSharedPointer<CompositeKey>::create();
    noRemember->addKey(QSharedPointer<PasswordKey>::create("password"));
    auto noRememberCr = QSharedPointer<ChallengeResponseKey>::create(slot);
    noRemember->addChallengeResponseKey(noRememberCr);
    {
        Database db;
        QString error;
        QVERIFY2(db.open(file, noRemember, &error), qPrintable(error));
    }
    QCOMPARE(prompts, 1);
    QVERIFY(!noRememberCr->hasCachedPin());
    auto restoredNoRemember = QSharedPointer<CompositeKey>::create();
    restoredNoRemember->setRawKey(noRemember->serialize());
    QVERIFY(!restoredNoRemember->challengeResponseKeys().first()->hasCachedPin());
}

void TestTpm2Swtpm::testQuickUnlockDataFollowsSettingChanges()
{
    // The setting is changed while a database is open: the data Quick Unlock stores must follow, otherwise a PIN
    // that is remembered on a Quick Unlock unlock is lost again, or a stale one stays in the stored data
    const YubiKeySlot slot{HANDLE_PIN_NODA, FIXED | PIN};
    config()->set(Config::Security_QuickUnlock, true);

    int prompts = 0;
    bool remember = false;
    YubiKey::setPinProvider(
        [&](const YubiKeySlot&, const QString&, Botan::secure_vector<char>& pin, bool& rememberOut) {
            ++prompts;
            pin = secure("1234");
            rememberOut = remember;
            return true;
        });

    const QString file = m_dir->filePath("settingchange.kdbx");
    auto newKey = [&] {
        auto key = QSharedPointer<CompositeKey>::create();
        key->addKey(QSharedPointer<PasswordKey>::create("password"));
        key->addChallengeResponseKey(QSharedPointer<ChallengeResponseKey>::create(slot));
        return key;
    };
    auto unlock = [&](const QSharedPointer<CompositeKey>& key) {
        Database db;
        QString error;
        QVERIFY2(db.open(file, key, &error), qPrintable(error));
    };
    auto restore = [](const QByteArray& stored) {
        auto key = QSharedPointer<CompositeKey>::create();
        key->setRawKey(stored);
        return key;
    };
    auto hasPin = [](const QSharedPointer<CompositeKey>& key) {
        return key->challengeResponseKeys().first()->hasCachedPin();
    };

    {
        Database db;
        QVERIFY(db.setKey(newKey()));
        QString error;
        QVERIFY2(db.saveAs(file, Database::Atomic, QString(), &error), qPrintable(error));
    }

    // 1. Unlock while the setting is off: the PIN is not kept, the stored data has none
    config()->set(Config::Security_QuickUnlockTpm2Pin, false);
    prompts = 0;
    remember = false;
    auto first = newKey();
    unlock(first);
    QCOMPARE(prompts, 1);
    QVERIFY(!first->quickUnlockDataOutdated());
    const auto storedWithoutPin = first->serialize();

    // 2. The setting is switched on. After locking, the database is unlocked with the stored data: it has no PIN, so
    //    the PIN is asked for and, because of the setting, remembered
    config()->set(Config::Security_QuickUnlockTpm2Pin, true);
    remember = true;
    prompts = 0;
    auto second = restore(storedWithoutPin);
    QVERIFY(!hasPin(second));
    QVERIFY(!second->quickUnlockDataOutdated());
    unlock(second);
    QCOMPARE(prompts, 1);
    QVERIFY(hasPin(second));
    // The stored data lacks that PIN: this is what makes DatabaseOpenWidget store it again
    QVERIFY(second->quickUnlockDataOutdated());
    QVERIFY(DatabaseOpenWidget::shouldStoreQuickUnlockData(false, true, second->quickUnlockDataOutdated()));
    const auto storedWithPin = second->serialize();
    QVERIFY(storedWithPin.size() > storedWithoutPin.size());

    // 3. The next unlock with the updated data does not ask for the PIN, and there is nothing left to update
    prompts = 0;
    auto third = restore(storedWithPin);
    QVERIFY(hasPin(third));
    QVERIFY(!third->quickUnlockDataOutdated());
    unlock(third);
    QCOMPARE(prompts, 0);
    QVERIFY(!DatabaseOpenWidget::shouldStoreQuickUnlockData(false, true, third->quickUnlockDataOutdated()));

    // 4. The setting is switched off again: the stored data still has the PIN, which must not stay there
    config()->set(Config::Security_QuickUnlockTpm2Pin, false);
    prompts = 0;
    remember = false;
    auto fourth = restore(storedWithPin);
    QVERIFY(!hasPin(fourth)); // not used any more
    QVERIFY(fourth->quickUnlockDataOutdated()); // and the stored data is out of date
    unlock(fourth);
    QCOMPARE(prompts, 1);
    QVERIFY(DatabaseOpenWidget::shouldStoreQuickUnlockData(false, true, fourth->quickUnlockDataOutdated()));
    const auto storedCleaned = fourth->serialize();
    QCOMPARE(storedCleaned.size(), storedWithoutPin.size());
    QVERIFY(!restore(storedCleaned)->quickUnlockDataOutdated());
    config()->set(Config::Security_QuickUnlockTpm2Pin, true);
}
