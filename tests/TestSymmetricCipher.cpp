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

#include "TestSymmetricCipher.h"

#include <QBuffer>
#include <QTest>

#include "tests.h"
#include "crypto/Crypto.h"
#include "crypto/SymmetricCipher.h"
#include "streams/SymmetricCipherStream.h"

QTEST_GUILESS_MAIN(TestSymmetricCipher)

void TestSymmetricCipher::initTestCase()
{
    QVERIFY(Crypto::init());
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

    SymmetricCipher cipher(SymmetricCipher::Aes256, SymmetricCipher::Cbc, SymmetricCipher::Encrypt,
                           key, iv);
    QCOMPARE(cipher.blockSize(), 16);

    QCOMPARE(cipher.process(plainText),
             cipherText);

    QBuffer buffer;
    SymmetricCipherStream stream(&buffer, SymmetricCipher::Aes256, SymmetricCipher::Cbc,
                                 SymmetricCipher::Encrypt, key, iv);
    buffer.open(QIODevice::WriteOnly);
    stream.open(QIODevice::WriteOnly);
    QVERIFY(stream.reset());

    buffer.reset();
    buffer.buffer().clear();
    stream.write(plainText.left(16));
    QCOMPARE(buffer.data(), cipherText.left(16));
    QVERIFY(stream.reset());
    // make sure padding is written
    QCOMPARE(buffer.data().size(), 32);

    buffer.reset();
    buffer.buffer().clear();
    stream.write(plainText.left(10));
    QVERIFY(buffer.data().isEmpty());

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

    // padded with 16 0x16 bytes
    QByteArray cipherTextPadded = cipherText + QByteArray::fromHex("3a3aa5e0213db1a9901f9036cf5102d2");
    QBuffer buffer(&cipherTextPadded);
    SymmetricCipherStream stream(&buffer, SymmetricCipher::Aes256, SymmetricCipher::Cbc,
                                 SymmetricCipher::Decrypt, key, iv);
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

void TestSymmetricCipher::testSalsa20()
{
    // http://www.ecrypt.eu.org/stream/svn/viewcvs.cgi/ecrypt/trunk/submissions/salsa20/full/verified.test-vectors?logsort=rev&rev=210&view=markup

    QByteArray key = QByteArray::fromHex("F3F4F5F6F7F8F9FAFBFCFDFEFF000102030405060708090A0B0C0D0E0F101112");
    QByteArray iv = QByteArray::fromHex("0000000000000000");

    SymmetricCipher cipher(SymmetricCipher::Salsa20, SymmetricCipher::Stream, SymmetricCipher::Encrypt, key, iv);

    QByteArray cipherTextA;
    for (int i = 0; i < 8; i++) {
        cipherTextA.append(cipher.process(QByteArray(64, '\0')));
    }
    cipher.reset();

    QByteArray cipherTextB = cipher.process(QByteArray(512, '\0'));
    cipher.reset();

    QByteArray expectedCipherText1;
    expectedCipherText1.append(QByteArray::fromHex("B4C0AFA503BE7FC29A62058166D56F8F"));
    expectedCipherText1.append(QByteArray::fromHex("5D27DC246F75B9AD8760C8C39DFD8749"));
    expectedCipherText1.append(QByteArray::fromHex("2D3B76D5D9637F009EADA14458A52DFB"));
    expectedCipherText1.append(QByteArray::fromHex("09815337E72672681DDDC24633750D83"));

    QByteArray expectedCipherText2;
    expectedCipherText2.append(QByteArray::fromHex("DBBA0683DF48C335A9802EEF02522563"));
    expectedCipherText2.append(QByteArray::fromHex("54C9F763C3FDE19131A6BB7B85040624"));
    expectedCipherText2.append(QByteArray::fromHex("B1D6CD4BF66D16F7482236C8602A6D58"));
    expectedCipherText2.append(QByteArray::fromHex("505EEDCCA0B77AED574AB583115124B9"));

    QByteArray expectedCipherText3;
    expectedCipherText3.append(QByteArray::fromHex("F0C5F98BAE05E019764EF6B65E0694A9"));
    expectedCipherText3.append(QByteArray::fromHex("04CB9EC9C10C297B1AB1A6052365BB78"));
    expectedCipherText3.append(QByteArray::fromHex("E55D3C6CB9F06184BA7D425A92E7E987"));
    expectedCipherText3.append(QByteArray::fromHex("757FC5D9AFD7082418DD64125CA6F2B6"));

    QByteArray expectedCipherText4;
    expectedCipherText4.append(QByteArray::fromHex("5A5FB5C8F0AFEA471F0318A4A2792F7A"));
    expectedCipherText4.append(QByteArray::fromHex("A5C67B6D6E0F0DDB79961C34E3A564BA"));
    expectedCipherText4.append(QByteArray::fromHex("2EECE78D9AFF45E510FEAB1030B102D3"));
    expectedCipherText4.append(QByteArray::fromHex("9DFCECB77F5798F7D2793C0AB09C7A04"));

    QCOMPARE(cipherTextA.mid(0, 64), expectedCipherText1);
    QCOMPARE(cipherTextA.mid(192, 64), expectedCipherText2);
    QCOMPARE(cipherTextA.mid(256, 64), expectedCipherText3);
    QCOMPARE(cipherTextA.mid(448, 64), expectedCipherText4);

    QCOMPARE(cipherTextB.mid(0, 64), expectedCipherText1);
    QCOMPARE(cipherTextB.mid(192, 64), expectedCipherText2);
    QCOMPARE(cipherTextB.mid(256, 64), expectedCipherText3);
    QCOMPARE(cipherTextB.mid(448, 64), expectedCipherText4);
}

void TestSymmetricCipher::testPadding()
{
    QByteArray key = QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
    QByteArray iv = QByteArray::fromHex("000102030405060708090a0b0c0d0e0f");
    QByteArray plainText = QByteArray::fromHex("6bc1bee22e409f96e93d");

    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);

    SymmetricCipherStream streamEnc(&buffer, SymmetricCipher::Aes256, SymmetricCipher::Cbc,
                                    SymmetricCipher::Encrypt, key, iv);
    streamEnc.open(QIODevice::WriteOnly);
    streamEnc.write(plainText);
    streamEnc.close();
    buffer.reset();
    // make sure padding is written
    QCOMPARE(buffer.buffer().size(), 16);

    SymmetricCipherStream streamDec(&buffer, SymmetricCipher::Aes256, SymmetricCipher::Cbc,
                                    SymmetricCipher::Decrypt, key, iv);
    streamDec.open(QIODevice::ReadOnly);
    QByteArray decrypted = streamDec.readAll();
    QCOMPARE(decrypted, plainText);
}
