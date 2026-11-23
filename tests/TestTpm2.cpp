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

#include "TestTpm2.h"

#include "core/AsyncTask.h"
#include "core/Config.h"
#include "crypto/Crypto.h"
#include "gui/ApplicationSettingsWidget.h"
#include "gui/DatabaseOpenWidget.h"
#include "keys/ChallengeResponseKey.h"
#include "keys/CompositeKey.h"
#include "keys/PasswordKey.h"
#include "keys/drivers/YubiKey.h"
#include "keys/drivers/YubiKeyInterfaceTPM.h"

#include <QCoreApplication>
#include <QTest>
#include <QThread>
#include <QtConcurrent>

QTEST_GUILESS_MAIN(TestTpm2)

namespace
{
    const YubiKeySlot PIN_SLOT{0x81010005u, YubiKey::TPM_SLOT_FLAG | YubiKey::TPM_PIN_FLAG | YubiKey::TPM_MODE_FIXED64};
    const YubiKeySlot PLAIN_SLOT{0x81010004u, YubiKey::TPM_SLOT_FLAG | YubiKey::TPM_MODE_VARIABLE};

    Botan::secure_vector<char> secure(const QByteArray& text)
    {
        return Botan::secure_vector<char>(text.constData(), text.constData() + text.size());
    }

    QByteArray bytes(const Botan::secure_vector<char>& v)
    {
        return QByteArray(v.data(), static_cast<qsizetype>(v.size()));
    }

    QByteArray seq(int length)
    {
        QByteArray b(length, 0);
        for (int i = 0; i < length; ++i) {
            b[i] = static_cast<char>(i);
        }
        return b;
    }

    void setQuickUnlock(bool enabled, bool savePin)
    {
        config()->set(Config::Security_QuickUnlock, enabled);
        config()->set(Config::Security_QuickUnlockTpm2Pin, savePin);
    }
} // namespace

void TestTpm2::initTestCase()
{
    QVERIFY(Crypto::init());
    QLocale::setDefault(QLocale::c());

    // These tests must never talk to a TPM of the machine they run on: make every TCTI lookup fail
    qputenv("TPM2TOOLS_TCTI", "device:/nonexistent/tpm");

    // Both the shared and the local (machine specific) settings must live in temporary files, otherwise the
    // test would read and overwrite the user's real settings (e.g. Security/EnableTpm2, Security/QuickUnlock)
    QVERIFY(m_configFile.open());
    QVERIFY(m_localConfigFile.open());
    Config::createConfigFromFile(m_configFile.fileName(), m_localConfigFile.fileName());
    config()->set(Config::Security_EnableTpm2, false);
    setQuickUnlock(true, false);
}

void TestTpm2::cleanupTestCase()
{
    qunsetenv("TPM2TOOLS_TCTI");
}

void TestTpm2::cleanup()
{
    YubiKey::setPinProvider({});
    setQuickUnlock(true, false);
    config()->set(Config::Security_EnableTpm2, false);
}

void TestTpm2::testSlotHelpers_data()
{
    QTest::addColumn<int>("slotValue");
    QTest::addColumn<bool>("isTpm");
    QTest::addColumn<bool>("needsPin");
    QTest::addColumn<int>("mode");
    QTest::addColumn<bool>("validForCli");

    // Real YubiKey slots are never mistaken for TPM keys
    QTest::newRow("yubikey slot 1") << 1 << false << false << 1 << false;
    QTest::newRow("yubikey slot 2") << 2 << false << false << 2 << false;
    QTest::newRow("tpm fixed") << 0x101 << true << false << 1 << true;
    QTest::newRow("tpm variable") << 0x102 << true << false << 2 << true;
    QTest::newRow("tpm fixed pin") << 0x301 << true << true << 1 << true;
    QTest::newRow("tpm variable pin") << 0x302 << true << true << 2 << true;
    // Command line values: 257/258 and 769/770
    QTest::newRow("cli 257") << 257 << true << false << 1 << true;
    QTest::newRow("cli 770") << 770 << true << true << 2 << true;
    // Not valid TPM slots
    QTest::newRow("tpm mode 0") << 0x100 << true << false << 0 << false;
    QTest::newRow("tpm mode 3") << 0x103 << true << false << 3 << false;
    QTest::newRow("unknown high bit") << 0x501 << true << false << 1 << false;
}

void TestTpm2::testSlotHelpers()
{
    QFETCH(int, slotValue);
    QFETCH(bool, isTpm);
    QFETCH(bool, needsPin);
    QFETCH(int, mode);
    QFETCH(bool, validForCli);

    const YubiKeySlot slot{0x81010004u, slotValue};
    QCOMPARE(YubiKey::isTpmSlot(slot), isTpm);
    QCOMPARE(YubiKey::tpmSlotNeedsPin(slot), needsPin);
    QCOMPARE(YubiKey::tpmMode(slot), mode);
    QCOMPARE(YubiKey::isValidTpmSlotValue(slotValue), validForCli);
}

void TestTpm2::testBuildMessage_data()
{
    QTest::addColumn<QByteArray>("challenge");
    QTest::addColumn<int>("mode");
    QTest::addColumn<QByteArray>("expected");

    const int fixed = YubiKey::TPM_MODE_FIXED64;
    const int variable = YubiKey::TPM_MODE_VARIABLE;

    // Fixed mode: PKCS7-style padding up to 64 bytes
    QTest::newRow("fixed 32 bytes") << seq(32) << fixed << (seq(32) + QByteArray(32, 0x20));
    QTest::newRow("fixed 1 byte") << QByteArray("a") << fixed << (QByteArray("a") + QByteArray(63, 63));
    QTest::newRow("fixed empty") << QByteArray() << fixed << QByteArray(64, 64);
    QTest::newRow("fixed 63 bytes") << seq(63) << fixed << (seq(63) + QByteArray(1, 1));
    QTest::newRow("fixed 64 bytes untouched") << seq(64) << fixed << seq(64);
    QTest::newRow("fixed longer than 64 truncated") << seq(70) << fixed << seq(64);

    // Variable length (HMAC_LT64): the trailing run of the last byte is removed again
    QTest::newRow("variable 32 bytes") << seq(32) << variable << seq(32);
    QTest::newRow("variable short") << QByteArray("abc") << variable << QByteArray("abc");
    // A challenge that ends in the padding byte loses that byte too, exactly like on a YubiKey
    QByteArray endsInPad = seq(32);
    endsInPad[31] = 0x20;
    QTest::newRow("variable ends in pad byte") << endsInPad << variable << endsInPad.left(31);
    // Even without any padding the trailing run of the final byte is dropped (here just the single 0x3f)
    QTest::newRow("variable 64 bytes distinct end") << seq(64) << variable << seq(63);
    QTest::newRow("variable 64 identical bytes") << QByteArray(64, 'z') << variable << QByteArray();
    QTest::newRow("variable empty") << QByteArray() << variable << QByteArray();
}

void TestTpm2::testBuildMessage()
{
    QFETCH(QByteArray, challenge);
    QFETCH(int, mode);
    QFETCH(QByteArray, expected);

    QCOMPARE(YubiKeyInterfaceTPM::buildMessage(challenge, mode), expected);
}

void TestTpm2::testPinFromString_data()
{
    QTest::addColumn<QString>("text");

    QTest::newRow("empty") << QString();
    QTest::newRow("ascii") << QStringLiteral("1234");
    QTest::newRow("accents and euro") << QString::fromUtf8("p\xc3\xa4ssw\xc3\xb6rd\xe2\x82\xac");
    QTest::newRow("cjk and emoji") << QString::fromUtf8("\xe6\xbc\xa2\xe5\xad\x97\xf0\x9f\x98\x80x");
    QTest::newRow("long") << QString(300, QLatin1Char('a'));
}

void TestTpm2::testPinFromString()
{
    QFETCH(QString, text);
    QCOMPARE(bytes(YubiKey::pinFromString(text)), text.toUtf8());
}

void TestTpm2::testPinProvider()
{
    const auto slot = PIN_SLOT;
    Botan::secure_vector<char> pin;
    bool remember = true;

    // Without a provider nothing can be asked
    QVERIFY(!YubiKey::requestPin(slot, QString(), pin, remember));

    int calls = 0;
    YubiKeySlot seenSlot;
    QString seenMessage;
    YubiKey::setPinProvider([&](const YubiKeySlot& s, const QString& message, Botan::secure_vector<char>& p, bool& r) {
        ++calls;
        seenSlot = s;
        seenMessage = message;
        p = secure("1234");
        r = true;
        return true;
    });

    remember = false;
    QVERIFY(YubiKey::requestPin(slot, QStringLiteral("previous failure"), pin, remember));
    QCOMPARE(calls, 1);
    QCOMPARE(seenSlot, slot);
    QCOMPARE(seenMessage, QStringLiteral("previous failure"));
    QCOMPARE(bytes(pin), QByteArray("1234"));
    QVERIFY(remember);

    // A cancelled prompt is reported as such
    YubiKey::setPinProvider(
        [](const YubiKeySlot&, const QString&, Botan::secure_vector<char>&, bool&) { return false; });
    QVERIFY(!YubiKey::requestPin(slot, QString(), pin, remember));
}

void TestTpm2::testPinProviderRunsOnMainThread()
{
    // A prompt requested from a worker thread (e.g. a save running in the background) must still be shown
    // from the GUI thread
    QThread* providerThread = nullptr;
    YubiKey::setPinProvider(
        [&](const YubiKeySlot&, const QString&, Botan::secure_vector<char>& p, bool& r) {
            providerThread = QThread::currentThread();
            p = secure("1234");
            r = false;
            return true;
        });

    bool result = false;
    Botan::secure_vector<char> pin;
    bool remember = true;
    auto future = QtConcurrent::run([&] { result = YubiKey::requestPin(PIN_SLOT, QString(), pin, remember); });
    QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(), 5000);

    QVERIFY(result);
    QCOMPARE(providerThread, QCoreApplication::instance()->thread());
    QCOMPARE(bytes(pin), QByteArray("1234"));
    QVERIFY(!remember);
}

void TestTpm2::testQuickUnlockPinSerialization()
{
    setQuickUnlock(true, true);

    ChallengeResponseKey key(PIN_SLOT);
    key.setCachedPin(secure("1234"));
    QVERIFY(key.needsPin());
    QVERIFY(key.hasCachedPin());

    const auto blob = key.serialize();

    ChallengeResponseKey restored;
    restored.deserialize(blob);
    QCOMPARE(restored.slotData(), PIN_SLOT);
    QVERIFY(restored.needsPin());
    QVERIFY(restored.hasCachedPin());

    // Clearing really forgets the PIN
    restored.clearCachedPin();
    QVERIFY(!restored.hasCachedPin());

    // A PIN key without a remembered PIN stores none
    ChallengeResponseKey noPin(PIN_SLOT);
    ChallengeResponseKey restoredNoPin;
    restoredNoPin.deserialize(noPin.serialize());
    QCOMPARE(restoredNoPin.slotData(), PIN_SLOT);
    QVERIFY(!restoredNoPin.hasCachedPin());

    // Keys that do not use a PIN never carry one, even if one was set
    ChallengeResponseKey plain(PLAIN_SLOT);
    plain.setCachedPin(secure("1234"));
    ChallengeResponseKey restoredPlain;
    restoredPlain.deserialize(plain.serialize());
    QCOMPARE(restoredPlain.slotData(), PLAIN_SLOT);
    QVERIFY(!restoredPlain.hasCachedPin());

    // Damaged data neither crashes nor yields a PIN
    ChallengeResponseKey damaged;
    damaged.deserialize(blob.left(blob.size() - 3));
    QVERIFY(!damaged.hasCachedPin());
}

void TestTpm2::testQuickUnlockPinIgnoredWithoutOptIn()
{
    ChallengeResponseKey key(PIN_SLOT);
    key.setCachedPin(secure("1234"));

    // Option off: the slot is saved, the PIN is not
    setQuickUnlock(true, false);
    const auto withoutPin = key.serialize();
    ChallengeResponseKey a;
    a.deserialize(withoutPin);
    QCOMPARE(a.slotData(), PIN_SLOT);
    QVERIFY(!a.hasCachedPin());

    // Quick unlock itself disabled: the option alone is not enough
    setQuickUnlock(false, true);
    QCOMPARE(key.serialize().size(), withoutPin.size());

    // Data saved with the option on is ignored once the user turns the option off again
    setQuickUnlock(true, true);
    const auto withPin = key.serialize();
    QVERIFY(withPin.size() > withoutPin.size());
    setQuickUnlock(true, false);
    ChallengeResponseKey b;
    b.deserialize(withPin);
    QCOMPARE(b.slotData(), PIN_SLOT);
    QVERIFY(!b.hasCachedPin());
}

void TestTpm2::testCompositeKeyPinLifecycle()
{
    setQuickUnlock(true, true);

    auto composite = QSharedPointer<CompositeKey>::create();
    composite->addKey(QSharedPointer<PasswordKey>::create("password"));
    auto crKey = QSharedPointer<ChallengeResponseKey>::create(PIN_SLOT);
    crKey->setCachedPin(secure("1234"));
    composite->addChallengeResponseKey(crKey);

    // This is what Quick Unlock stores and later restores
    CompositeKey restored;
    restored.setRawKey(composite->serialize());
    QCOMPARE(restored.keys().size(), 1);
    QCOMPARE(restored.challengeResponseKeys().size(), 1);
    auto restoredCr = restored.challengeResponseKeys().first();
    QCOMPARE(restoredCr->slotData(), PIN_SLOT);
    QVERIFY(restoredCr->hasCachedPin());

    // Locking/closing wipes the remembered PIN of every challenge-response key
    restored.clearCachedSecrets();
    QVERIFY(!restoredCr->hasCachedPin());
    QVERIFY(crKey->hasCachedPin());
    composite->clearCachedSecrets();
    QVERIFY(!crKey->hasCachedPin());
}

void TestTpm2::testQuickUnlockTpm2PinOptionAvailability_data()
{
    QTest::addColumn<bool>("quickUnlockAvailable");
    QTest::addColumn<bool>("quickUnlockEnabled");
    QTest::addColumn<bool>("tpm2Enabled");
    QTest::addColumn<bool>("managed");
    QTest::addColumn<QString>("mentions"); // empty: the option can be changed

    QTest::newRow("all enabled") << true << true << true << false << QString();
    QTest::newRow("TPM2 keys off") << true << true << false << false << "TPM2 challenge-response keys";
    QTest::newRow("Quick Unlock off") << true << false << true << false << "Quick Unlock is turned off";
    QTest::newRow("Quick Unlock unavailable") << false << true << true << false << "not available";
    QTest::newRow("both missing") << true << false << false << false << "Quick Unlock is turned off";
    QTest::newRow("managed") << true << true << true << true << "administrator";
}

void TestTpm2::testQuickUnlockTpm2PinOptionAvailability()
{
    QFETCH(bool, quickUnlockAvailable);
    QFETCH(bool, quickUnlockEnabled);
    QFETCH(bool, tpm2Enabled);
    QFETCH(bool, managed);
    QFETCH(QString, mentions);

    const auto reason = ApplicationSettingsWidget::quickUnlockTpm2PinUnavailableReason(
        quickUnlockAvailable, quickUnlockEnabled, tpm2Enabled, managed);
    if (mentions.isEmpty()) {
        QVERIFY2(reason.isEmpty(), qPrintable(reason));
    } else {
        QVERIFY2(reason.contains(mentions), qPrintable(reason));
    }

    // Both missing prerequisites are named
    if (!quickUnlockEnabled && !tpm2Enabled && !managed) {
        QVERIFY(reason.contains("TPM2 challenge-response keys"));
    }
}

void TestTpm2::testQuickUnlockStoreDecision_data()
{
    QTest::addColumn<bool>("blockedByCaller");
    QTest::addColumn<bool>("onQuickUnlockScreen");
    QTest::addColumn<bool>("dataOutdated");
    QTest::addColumn<bool>("store");

    // A regular unlock stores the data
    QTest::newRow("regular unlock") << false << false << false << true;
    QTest::newRow("regular unlock, outdated") << false << false << true << true;
    // Unlocking with the stored data leaves it alone ...
    QTest::newRow("quick unlock") << false << true << false << false;
    // ... unless it no longer matches (e.g. a TPM2 PIN was remembered on this unlock)
    QTest::newRow("quick unlock, outdated") << false << true << true << true;
    // Explicit credentials from the caller never touch Quick Unlock
    QTest::newRow("blocked") << true << false << false << false;
    QTest::newRow("blocked, outdated") << true << false << true << false;
    QTest::newRow("blocked, quick unlock, outdated") << true << true << true << false;
}

void TestTpm2::testQuickUnlockStoreDecision()
{
    QFETCH(bool, blockedByCaller);
    QFETCH(bool, onQuickUnlockScreen);
    QFETCH(bool, dataOutdated);
    QFETCH(bool, store);

    QCOMPARE(DatabaseOpenWidget::shouldStoreQuickUnlockData(blockedByCaller, onQuickUnlockScreen, dataOutdated), store);
}

void TestTpm2::testPinSlotWithoutPinNeverContactsTpm()
{
    // The TPM connection of this test is deliberately unusable, so reaching the TPM would give a connection
    // error. A PIN slot without a PIN must be refused before that, because an empty password would count as a
    // failed attempt towards the dictionary-attack lockout of a real TPM.
    Botan::secure_vector<char> response;
    auto* tpm = YubiKeyInterfaceTPM::instance();

    QCOMPARE(tpm->challengeWithPin(PIN_SLOT, QByteArray(32, 'x'), response, nullptr),
             YubiKey::ChallengeResult::YCR_ERROR);
    QVERIFY2(tpm->errorMessage().contains("required"), qPrintable(tpm->errorMessage()));

    Botan::secure_vector<char> emptyPin;
    QCOMPARE(tpm->challengeWithPin(PIN_SLOT, QByteArray(32, 'x'), response, &emptyPin),
             YubiKey::ChallengeResult::YCR_ERROR);
    QVERIFY2(tpm->errorMessage().contains("required"), qPrintable(tpm->errorMessage()));

    bool ok = true;
    YubiKey::ChallengeResult result = YubiKey::ChallengeResult::YCR_SUCCESS;
    ok = tpm->testChallengeWithPin(PIN_SLOT, nullptr, nullptr, &result);
    QVERIFY(!ok);
    QCOMPARE(result, YubiKey::ChallengeResult::YCR_ERROR);
}

void TestTpm2::testKeyPromptCancelled()
{
    config()->set(Config::Security_EnableTpm2, true);

    // No provider at all behaves like a cancelled prompt
    ChallengeResponseKey key(PIN_SLOT);
    QVERIFY(!key.challenge(QByteArray(32, 'x')));
    QVERIFY2(key.error().contains("cancelled"), qPrintable(key.error()));
    QVERIFY(key.rawKey().isEmpty());

    int calls = 0;
    YubiKey::setPinProvider([&](const YubiKeySlot&, const QString&, Botan::secure_vector<char>&, bool&) {
        ++calls;
        return false;
    });
    QVERIFY(!key.challenge(QByteArray(32, 'x')));
    QCOMPARE(calls, 1);
    QVERIFY2(key.error().contains("cancelled"), qPrintable(key.error()));
    QVERIFY(!key.hasCachedPin());
}

void TestTpm2::testKeyPromptNonAuthErrorStops()
{
    // With TPM2 disabled the challenge fails with an error that is not a wrong PIN: the user must be asked
    // once, the failure reported, and nothing remembered, even though "remember" was ticked
    config()->set(Config::Security_EnableTpm2, false);

    int calls = 0;
    YubiKey::setPinProvider(
        [&](const YubiKeySlot&, const QString&, Botan::secure_vector<char>& pin, bool& remember) {
            ++calls;
            pin = secure("1234");
            remember = true;
            return true;
        });

    ChallengeResponseKey key(PIN_SLOT);
    QVERIFY(!key.challenge(QByteArray(32, 'x')));
    QCOMPARE(calls, 1);
    QVERIFY2(key.error().contains("disabled"), qPrintable(key.error()));
    QVERIFY(!key.hasCachedPin());
    QVERIFY(key.rawKey().isEmpty());
}

void TestTpm2::testNoticeIsEmptyWhenTpmDisabled()
{
    config()->set(Config::Security_EnableTpm2, false);
    QVERIFY(YubiKey::instance()->pinNotice(PIN_SLOT).isEmpty());
    // Not a PIN key at all
    QVERIFY(YubiKey::instance()->pinNotice(PLAIN_SLOT).isEmpty());
}
