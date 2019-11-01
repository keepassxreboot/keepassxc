/*
 *  Copyright (C) 2019 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include "TestFdoSecrets.h"

#include "TestGlobal.h"

#include "core/EntrySearcher.h"
#include "fdosecrets/GcryptMPI.h"
#include "fdosecrets/objects/SessionCipher.h"
#include "fdosecrets/objects/Collection.h"
#include "fdosecrets/objects/Item.h"

#include "crypto/Crypto.h"

QTEST_GUILESS_MAIN(TestFdoSecrets)

void TestFdoSecrets::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestFdoSecrets::cleanupTestCase()
{
}

void TestFdoSecrets::testGcryptMPI()
{
    auto bytes = QByteArray::fromHex(QByteArrayLiteral("DEADBEEF"));

    auto mpi = MpiFromBytes(bytes);
    auto another = MpiFromHex("DEADBEEF");

    // verify it can parse the bytes in USG mode
    QVERIFY(mpi.get());
    QVERIFY(another.get());

    // verify the number is of the correct value
    QCOMPARE(gcry_mpi_cmp_ui(mpi.get(), 0xdeadbeef), 0);
    QCOMPARE(gcry_mpi_cmp_ui(another.get(), 0xdeadbeef), 0);

    // verify it can convert back
    QCOMPARE(MpiToBytes(mpi), bytes);
    QCOMPARE(MpiToBytes(another), bytes);
}

void TestFdoSecrets::testDhIetf1024Sha256Aes128CbcPkcs7()
{
    auto clientPublic = MpiFromHex("40a0c8d27012c651bf270ebd96890a538"
                                   "396fae3852aef69c0c19bae420d667577"
                                   "ed471cd8ba5a49ef0ec91b568b95f87f0"
                                   "9ec31d271f1699ed140c5b38644c42f60"
                                   "ef84b5a6c406e17c07cd3208e5a605626"
                                   "a5266153b447529946be2394dd43e5638"
                                   "5ffbc4322902c2942391d1a36e8d125dc"
                                   "809e3e406a2f5c2dcf39d3da2");
    auto serverPublic = MpiFromHex("e407997e8b918419cf851cf3345358fdf"
                                   "ffb9564a220ac9c3934efd277cea20d17"
                                   "467ecdc56e817f75ac39501f38a4a04ff"
                                   "64d627e16c09981c7ad876da255b61c8e"
                                   "6a8408236c2a4523cfe6961c26dbdfc77"
                                   "c1a27a5b425ca71a019e829fae32c0b42"
                                   "0e1b3096b48bc2ce9ccab1d1ff13a5eb4"
                                   "b263cee30bdb1a57af9bfa93f");
    auto serverPrivate = MpiFromHex("013f4f3381ef0ca11c4c7363079577b56"
                                    "99b238644e0aba47e24bdba6173590216"
                                    "4f1e12dd0944800a373e090e63192f53b"
                                    "93583e9a9e50bb9d792aafaa3a0f5ae77"
                                    "de0c3423f5820848d88ee3bdd01c889f2"
                                    "7af58a02f5b6693d422b9d189b300d7b1"
                                    "be5076b5795cf8808c31e2e2898368d18"
                                    "ab5c26b0ea3480c9aba8154cf");

    std::unique_ptr<FdoSecrets::DhIetf1024Sha256Aes128CbcPkcs7> cipher{new FdoSecrets::DhIetf1024Sha256Aes128CbcPkcs7};

    cipher->initialize(std::move(clientPublic), std::move(serverPublic), std::move(serverPrivate));

    QVERIFY(cipher->isValid());

    QCOMPARE(cipher->m_aesKey.toHex(), QByteArrayLiteral("6b8f5ee55138eac37118508be21e7834"));
}

void TestFdoSecrets::testCrazyAttributeKey()
{
    using FdoSecrets::Item;
    using FdoSecrets::Collection;

    const QScopedPointer<Group> root(new Group());
    const QScopedPointer<Entry> e1(new Entry());
    e1->setGroup(root.data());

    const QString key = "_a:bc&-+'-e%12df_d";
    const QString value = "value";
    e1->attributes()->set(key, value);

    // search for custom entries
    const auto term = Collection::attributeToTerm(key, value);
    const auto res = EntrySearcher().search({term}, root.data());
    QCOMPARE(res.count(), 1);
}
