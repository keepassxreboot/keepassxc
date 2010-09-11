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

#include <QtTest/QTest>

#include "crypto/Crypto.h"
#include "crypto/SymmetricCipher.h"

class TestSymmetricCipher : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testAes256CbcEncryption();
};

void TestSymmetricCipher::initTestCase()
{
    Crypto::init();
}

void TestSymmetricCipher::testAes256CbcEncryption()
{
    // http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf

    QByteArray key = QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
    QByteArray iv = QByteArray::fromHex("000102030405060708090a0b0c0d0e0f");
    QByteArray plain = QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172a");

    SymmetricCipher encrypt(SymmetricCipher::Aes256, SymmetricCipher::Cbc, SymmetricCipher::Encrypt, key, iv);
    QCOMPARE(QString(encrypt.process(plain).toHex()),
             QString("f58c4c04d6e5f1ba779eabfb5f7bfbd6"));
}

QTEST_MAIN(TestSymmetricCipher);

#include "TestSymmetricCipher.moc"
