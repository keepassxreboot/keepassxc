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

#include <QtCore/QBuffer>
#include <QtTest/QTest>

#include "crypto/Crypto.h"
#include "crypto/SymmetricCipher.h"
#include "streams/SymmetricCipherStream.h"

class TestSymmetricCipher : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testAes256CbcEncryption();
    void testAes256CbcDecryption();
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
    QByteArray plainText = QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172a");
    plainText.append(QByteArray::fromHex("ae2d8a571e03ac9c9eb76fac45af8e51"));
    QByteArray cipherText = QByteArray::fromHex("f58c4c04d6e5f1ba779eabfb5f7bfbd6");
    cipherText.append(QByteArray::fromHex("9cfc4e967edb808d679f777bc6702c7d"));

    SymmetricCipher cipher(SymmetricCipher::Aes256, SymmetricCipher::Cbc, SymmetricCipher::Encrypt, key, iv);
    QCOMPARE(cipher.blockSize(), 16);

    QCOMPARE(cipher.process(plainText),
             cipherText);

    QBuffer buffer;
    SymmetricCipherStream stream(&buffer, SymmetricCipher::Aes256, SymmetricCipher::Cbc, SymmetricCipher::Encrypt, key, iv);
    buffer.open(QIODevice::WriteOnly);
    stream.open(QIODevice::WriteOnly);

    QVERIFY(stream.reset());
    buffer.reset();
    buffer.buffer().clear();
    stream.write(plainText.left(16));
    QCOMPARE(buffer.data(), cipherText.left(16));

    QVERIFY(stream.reset());
    buffer.reset();
    buffer.buffer().clear();
    stream.write(plainText.left(10));
    QCOMPARE(buffer.data(), QByteArray());

    QVERIFY(stream.reset());
    buffer.reset();
    buffer.buffer().clear();
    stream.write(plainText.left(10));
    stream.close();
    QCOMPARE(buffer.data().size(), 16);
}

void TestSymmetricCipher::testAes256CbcDecryption()
{
    QByteArray key = QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
    QByteArray iv = QByteArray::fromHex("000102030405060708090a0b0c0d0e0f");
    QByteArray cipherText = QByteArray::fromHex("f58c4c04d6e5f1ba779eabfb5f7bfbd6");
    cipherText.append(QByteArray::fromHex("9cfc4e967edb808d679f777bc6702c7d"));
    QByteArray plainText = QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172a");
    plainText.append(QByteArray::fromHex("ae2d8a571e03ac9c9eb76fac45af8e51"));

    SymmetricCipher cipher(SymmetricCipher::Aes256, SymmetricCipher::Cbc, SymmetricCipher::Decrypt, key, iv);
    QCOMPARE(cipher.blockSize(), 16);

    QCOMPARE(cipher.process(cipherText),
             plainText);

    QBuffer buffer(&cipherText);
    SymmetricCipherStream stream(&buffer, SymmetricCipher::Aes256, SymmetricCipher::Cbc, SymmetricCipher::Decrypt, key, iv);
    buffer.open(QIODevice::ReadOnly);
    stream.open(QIODevice::ReadOnly);

    QCOMPARE(stream.read(10),
             plainText.left(10));
    buffer.reset();
    stream.reset();
    QCOMPARE(stream.read(20),
             plainText.left(20));
    buffer.reset();
    stream.reset();
    QCOMPARE(stream.read(16),
             plainText.left(16));
    buffer.reset();
    stream.reset();
    QCOMPARE(stream.read(100),
             plainText);
}

QTEST_MAIN(TestSymmetricCipher);

#include "TestSymmetricCipher.moc"
