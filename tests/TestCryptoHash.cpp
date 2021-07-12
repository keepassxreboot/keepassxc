/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "TestCryptoHash.h"

#include "crypto/Crypto.h"
#include "crypto/CryptoHash.h"

#include <QTest>

QTEST_GUILESS_MAIN(TestCryptoHash)

void TestCryptoHash::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestCryptoHash::test()
{
    CryptoHash cryptoHash1(CryptoHash::Sha256);
    QCOMPARE(cryptoHash1.result(),
             QByteArray::fromHex("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));

    QByteArray source2 = QString("KeePassX").toLatin1();
    QByteArray result2 = CryptoHash::hash(source2, CryptoHash::Sha256);
    QCOMPARE(result2, QByteArray::fromHex("0b56e5f65263e747af4a833bd7dd7ad26a64d7a4de7c68e52364893dca0766b4"));

    CryptoHash cryptoHash3(CryptoHash::Sha256);
    cryptoHash3.addData(QString("KeePa").toLatin1());
    cryptoHash3.addData(QString("ssX").toLatin1());
    QCOMPARE(cryptoHash3.result(),
             QByteArray::fromHex("0b56e5f65263e747af4a833bd7dd7ad26a64d7a4de7c68e52364893dca0766b4"));

    CryptoHash cryptoHash2(CryptoHash::Sha512);
    QCOMPARE(cryptoHash2.result(),
             QByteArray::fromHex("cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff831"
                                 "8d2877eec2f63b931bd47417a81a538327af927da3e"));

    QByteArray result3 = CryptoHash::hash(source2, CryptoHash::Sha512);
    QCOMPARE(result3,
             QByteArray::fromHex("0d41b612584ed39ff72944c29494573e40f4bb95283455fae2e0be1e3565aa9f48057d59e6ff"
                                 "d777970e282871c25a549a2763e5b724794f312c97021c42f91d"));

    CryptoHash cryptoHash4(CryptoHash::Sha512);
    cryptoHash4.addData(QString("KeePa").toLatin1());
    cryptoHash4.addData(QString("ssX").toLatin1());
    QCOMPARE(cryptoHash4.result(),
             QByteArray::fromHex("0d41b612584ed39ff72944c29494573e40f4bb95283455fae2e0be1e3565aa9f48057d59e6ffd777970e2"
                                 "82871c25a549a2763e5b724794f312c97021c42f91d"));
}
