/*
**  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
*  Copyright (C) 2014 Kyle Manna <kyle@kylemanna.com>
*
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

#include "TestYkChallengeResponseKey.h"

#include "core/Config.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "keys/ChallengeResponseKey.h"

#include <QCryptographicHash>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTest>

#include <algorithm>

QTEST_GUILESS_MAIN(TestYubiKeyChallengeResponse)

void TestYubiKeyChallengeResponse::initTestCase()
{
    // crypto subsystem needs to be initialized for YubiKey testing
    QVERIFY(Crypto::init());
    QLocale::setDefault(QLocale::c());

    // This test is about YubiKeys with a known secret. Use throw-away settings with TPM2 keys disabled, so
    // that it neither depends on the user's settings nor ever challenges a TPM.
    QVERIFY(m_configFile.open());
    QVERIFY(m_localConfigFile.open());
    Config::createConfigFromFile(m_configFile.fileName(), m_localConfigFile.fileName());
    config()->set(Config::Security_EnableTpm2, false);

    if (!YubiKey::instance()->isInitialized()) {
        QSKIP("Unable to initialize YubiKey interface.");
    }
}

void TestYubiKeyChallengeResponse::testDetectDevices()
{
    YubiKey::instance()->findValidKeys();

    // Look at the information retrieved from the key(s)
    const auto foundKeys = YubiKey::instance()->foundKeys();
    QRegularExpression exp{"\\w+\\s+\\[\\d+\\]\\s+-\\s+Slot\\s+\\d"};

    for (auto i = foundKeys.cbegin(); i != foundKeys.cend(); ++i) {
        QVERIFY(!YubiKey::isTpmSlot(i.key()));
        const auto& displayName = i.value();
        auto match = exp.match(displayName);
        QVERIFY(match.hasMatch());
        QVERIFY(displayName.contains(QString::number(i.key().first)));
        QVERIFY(displayName.contains(QString::number(i.key().second)));
    }
}

/**
 * Secret key for the YubiKey slot used by the unit test is
 * 1c e3 0f d7 8d 20 dc fa 40 b5 0c 18 77 9a fb 0f 02 28 8d b7
 * This secret can be on either slot but must be passive.
 */
void TestYubiKeyChallengeResponse::testKeyChallenge()
{
    auto keys = YubiKey::instance()->foundKeys().keys();
    keys.erase(std::remove_if(keys.begin(), keys.end(), [](const YubiKeySlot& k) { return YubiKey::isTpmSlot(k); }),
               keys.end());
    if (keys.isEmpty()) {
        QSKIP("No YubiKey devices were detected.");
    }

    // Find a key that is configured in passive mode
    bool wouldBlock = false;
    YubiKeySlot pKey(0, 0);
    for (auto key : keys) {
        if (YubiKey::instance()->testChallenge(key, &wouldBlock) && !wouldBlock) {
            pKey = key;
            break;
        }
        Tools::wait(100);
    }

    if (pKey.first == 0) {
        /* Testing active mode in unit tests is unreasonable */
        QSKIP("No YubiKey contains a slot in passive mode.");
    }

    QScopedPointer<ChallengeResponseKey> key(new ChallengeResponseKey(pKey));

    QByteArray ba("UnitTest");
    QVERIFY(key->challenge(ba));
    QCOMPARE(key->rawKey().size(), 20);
    auto hash = QString(QCryptographicHash::hash(key->rawKey(), QCryptographicHash::Sha256).toHex());
    QCOMPARE(hash, QString("2f7802c7112c301303526e7737b54d546c905076dca6e9538edf761a2264cd70"));
}
