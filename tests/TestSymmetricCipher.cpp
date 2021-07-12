/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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
#include <QVector>

#include "crypto/Crypto.h"
#include "format/KeePass2.h"
#include "streams/SymmetricCipherStream.h"

QTEST_GUILESS_MAIN(TestSymmetricCipher)
Q_DECLARE_METATYPE(SymmetricCipher::Mode);
Q_DECLARE_METATYPE(SymmetricCipher::Direction);

void TestSymmetricCipher::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestSymmetricCipher::testCipherUuidToMode()
{
    QCOMPARE(SymmetricCipher::cipherUuidToMode(KeePass2::CIPHER_AES128), SymmetricCipher::Aes128_CBC);
    QCOMPARE(SymmetricCipher::cipherUuidToMode(KeePass2::CIPHER_AES256), SymmetricCipher::Aes256_CBC);
    QCOMPARE(SymmetricCipher::cipherUuidToMode(KeePass2::CIPHER_TWOFISH), SymmetricCipher::Twofish_CBC);
    QCOMPARE(SymmetricCipher::cipherUuidToMode(KeePass2::CIPHER_CHACHA20), SymmetricCipher::ChaCha20);
    QCOMPARE(SymmetricCipher::cipherUuidToMode(QUuid()), SymmetricCipher::InvalidMode);
}

// clang-format off
void TestSymmetricCipher::testEncryptionDecryption_data()
{
    QTest::addColumn<SymmetricCipher::Mode>("mode");
    QTest::addColumn<SymmetricCipher::Direction>("direction");
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("iv");
    QTest::addColumn<QByteArray>("plainText");
    QTest::addColumn<QByteArray>("cipherText");

    // http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf
    QTest::newRow("AES128-CBC Encryption")
        << SymmetricCipher::Aes128_CBC
        << SymmetricCipher::Encrypt
        << QByteArray::fromHex("2b7e151628aed2a6abf7158809cf4f3c")
        << QByteArray::fromHex("000102030405060708090a0b0c0d0e0f")
        << QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172a")
        << QByteArray::fromHex("7649abac8119b246cee98e9b12e9197d5086cb9b507219ee95db113a917678b2");

    QTest::newRow("AES128-CBC Decryption")
        << SymmetricCipher::Aes128_CBC
        << SymmetricCipher::Decrypt
        << QByteArray::fromHex("2b7e151628aed2a6abf7158809cf4f3c")
        << QByteArray::fromHex("000102030405060708090a0b0c0d0e0f")
        << QByteArray::fromHex("7649abac8119b246cee98e9b12e9197d5086cb9b507219ee95db113a917678b2")
        << QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172a");

    QTest::newRow("AES256-CBC Encryption")
        << SymmetricCipher::Aes256_CBC
        << SymmetricCipher::Encrypt
        << QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4")
        << QByteArray::fromHex("000102030405060708090a0b0c0d0e0f")
        << QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172a")
        << QByteArray::fromHex("f58c4c04d6e5f1ba779eabfb5f7bfbd69cfc4e967edb808d679f777bc6702c7d");

    QTest::newRow("AES256-CBC Decryption")
        << SymmetricCipher::Aes256_CBC
        << SymmetricCipher::Decrypt
        << QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4")
        << QByteArray::fromHex("000102030405060708090a0b0c0d0e0f")
        << QByteArray::fromHex("f58c4c04d6e5f1ba779eabfb5f7bfbd69cfc4e967edb808d679f777bc6702c7d")
        << QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172a");

    QTest::newRow("AES256-CTR Encryption")
        << SymmetricCipher::Aes256_CTR
        << SymmetricCipher::Encrypt
        << QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4")
        << QByteArray::fromHex("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff")
        << QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172a")
        << QByteArray::fromHex("601ec313775789a5b7a7f504bbf3d228f443e3ca4d62b59aca84e990cacaf5c5");

    QTest::newRow("AES256-CTR Decryption")
        << SymmetricCipher::Aes256_CTR
        << SymmetricCipher::Decrypt
        << QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4")
        << QByteArray::fromHex("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff")
        << QByteArray::fromHex("601ec313775789a5b7a7f504bbf3d228f443e3ca4d62b59aca84e990cacaf5c5")
        << QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172a");
}
// clang-format on

void TestSymmetricCipher::testEncryptionDecryption()
{
    QFETCH(SymmetricCipher::Mode, mode);
    QFETCH(SymmetricCipher::Direction, direction);
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, iv);
    QFETCH(QByteArray, plainText);
    QFETCH(QByteArray, cipherText);

    QByteArray data = plainText;
    SymmetricCipher cipher;
    QVERIFY(cipher.init(mode, direction, key, iv));
    QVERIFY(cipher.process(data));
    QCOMPARE(data.left(16), cipherText.left(16));

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    SymmetricCipherStream stream(&buffer);
    QVERIFY(stream.init(mode, direction, key, iv));
    QVERIFY(stream.open(QIODevice::WriteOnly));
    QCOMPARE(stream.write(plainText.left(16)), qint64(16));
    stream.close();
    QCOMPARE(buffer.data().left(16), cipherText.left(16));
}

void TestSymmetricCipher::testAesCbcPadding_data()
{
    QTest::addColumn<SymmetricCipher::Mode>("mode");
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("iv");
    QTest::addColumn<QByteArray>("cipherText");
    QTest::addColumn<QByteArray>("plainText");
    QTest::addColumn<QByteArray>("padding");

    // clang-format off
    QTest::newRow("AES128") << SymmetricCipher::Aes128_CBC
                            << QByteArray::fromHex("2b7e151628aed2a6abf7158809cf4f3c")
                            << QByteArray::fromHex("000102030405060708090a0b0c0d0e0f")
                            << QByteArray::fromHex("7649abac8119b246cee98e9b12e9197d5086cb9b507219ee95db113a917678b2")
                            << QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51")
                            << QByteArray::fromHex("55e21d7100b988ffec32feeafaf23538");

    QTest::newRow("AES256") << SymmetricCipher::Aes256_CBC
                            << QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4")
                            << QByteArray::fromHex("000102030405060708090a0b0c0d0e0f")
                            << QByteArray::fromHex("f58c4c04d6e5f1ba779eabfb5f7bfbd69cfc4e967edb808d679f777bc6702c7d")
                            << QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51")
                            << QByteArray::fromHex("3a3aa5e0213db1a9901f9036cf5102d2");
    // clang-format on
}

void TestSymmetricCipher::testAesCbcPadding()
{
    QFETCH(SymmetricCipher::Mode, mode);
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, iv);
    QFETCH(QByteArray, cipherText);
    QFETCH(QByteArray, plainText);
    QFETCH(QByteArray, padding);

    // padded with 16 0x10 bytes
    QByteArray cipherTextPadded = cipherText + padding;

    QBuffer buffer(&cipherTextPadded);
    SymmetricCipherStream stream(&buffer);
    QVERIFY(stream.init(mode, SymmetricCipher::Decrypt, key, iv));
    buffer.open(QIODevice::ReadOnly);
    QVERIFY(stream.open(QIODevice::ReadOnly));

    QCOMPARE(stream.read(10), plainText.left(10));
    buffer.reset();
    QVERIFY(stream.reset());
    QVERIFY(stream.init(mode, SymmetricCipher::Decrypt, key, iv));
    QCOMPARE(stream.read(20), plainText.left(20));
    buffer.reset();
    QVERIFY(stream.reset());
    QVERIFY(stream.init(mode, SymmetricCipher::Decrypt, key, iv));
    QCOMPARE(stream.read(16), plainText.left(16));
    buffer.reset();
    QVERIFY(stream.reset());
    QVERIFY(stream.init(mode, SymmetricCipher::Decrypt, key, iv));
    QCOMPARE(stream.read(100), plainText);
}

void TestSymmetricCipher::testAesKdf()
{
    auto key = QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
    auto data = QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172a");
    auto result = QByteArray::fromHex("f3eed1bdb5d2a03c064b5a7e3db181f8");

    QVERIFY(SymmetricCipher::aesKdf(key, 1, data));
    QCOMPARE(data, result);

    // TODO: Test multiple rounds of AES KDF
}

void TestSymmetricCipher::testTwofish256CbcEncryption()
{
    // NIST MCT Known-Answer Tests (cbc_e_m.txt)
    // https://www.schneier.com/code/twofish-kat.zip

    QVector<QByteArray> keys{QByteArray::fromHex("0000000000000000000000000000000000000000000000000000000000000000"),
                             QByteArray::fromHex("D0A260EB41755B19374BABF259A79DB3EA7162E65490B03B1AE4871FB35EF23B"),
                             QByteArray::fromHex("8D55E4849A4DED08D89881E6708EDD26BEEE942073DFB3790B2798B240ACD74A"),
                             QByteArray::fromHex("606EFDC2066A837AF0430EBE4CF1F21071CCB236C33B4B9D82404FDB05C74621"),
                             QByteArray::fromHex("B119AA9485CEEEB4CC778AF21121E54DE4BDBA3498C61C8FD9004AA0C71909C3")};
    QVector<QByteArray> ivs{QByteArray::fromHex("00000000000000000000000000000000"),
                            QByteArray::fromHex("EA7162E65490B03B1AE4871FB35EF23B"),
                            QByteArray::fromHex("549FF6C6274F034211C31FADF3F22571"),
                            QByteArray::fromHex("CF222616B0E4F8E48967D769456B916B"),
                            QByteArray::fromHex("957108025BFD57125B40057BC2DE4FE2")};
    QVector<QByteArray> plainTexts{QByteArray::fromHex("00000000000000000000000000000000"),
                                   QByteArray::fromHex("D0A260EB41755B19374BABF259A79DB3"),
                                   QByteArray::fromHex("5DF7846FDB38B611EFD32A1429294095"),
                                   QByteArray::fromHex("ED3B19469C276E7228DB8F583C7F2F36"),
                                   QByteArray::fromHex("D177575683A46DCE3C34844C5DD0175D")};
    QVector<QByteArray> cipherTexts{QByteArray::fromHex("EA7162E65490B03B1AE4871FB35EF23B"),
                                    QByteArray::fromHex("549FF6C6274F034211C31FADF3F22571"),
                                    QByteArray::fromHex("CF222616B0E4F8E48967D769456B916B"),
                                    QByteArray::fromHex("957108025BFD57125B40057BC2DE4FE2"),
                                    QByteArray::fromHex("6F725C5950133F82EF021A94CADC8508")};

    SymmetricCipher cipher;

    for (int i = 0; i < keys.size(); ++i) {
        QVERIFY(cipher.init(SymmetricCipher::Twofish_CBC, SymmetricCipher::Encrypt, keys[i], ivs[i]));
        QByteArray ptNext = plainTexts[i];
        QByteArray ctPrev = ivs[i];
        QByteArray ctCur;
        QCOMPARE(cipher.blockSize(cipher.mode()), 16);
        for (int j = 0; j < 5000; ++j) {
            if (!cipher.process(ptNext)) {
                break;
            }
            ctCur = ptNext;
            ptNext = ctPrev;
            ctPrev = ctCur;

            if (!cipher.process(ptNext)) {
                break;
            }
            ctCur = ptNext;
            ptNext = ctPrev;
            ctPrev = ctCur;
        }

        QCOMPARE(ctCur, cipherTexts[i]);
    }
}

void TestSymmetricCipher::testTwofish256CbcDecryption()
{
    // NIST MCT Known-Answer Tests (cbc_d_m.txt)
    // https://www.schneier.com/code/twofish-kat.zip

    QVector<QByteArray> keys{QByteArray::fromHex("0000000000000000000000000000000000000000000000000000000000000000"),
                             QByteArray::fromHex("1B1FE8F5A911CD4C0D800EDCE8ED0A942CBA6271A1044F90C30BA8FE91E1C163"),
                             QByteArray::fromHex("EBA31FF8D2A24FDD769A937353E23257294A33394E4D17A668060AD8230811A1"),
                             QByteArray::fromHex("1DCF1915C389AB273F80F897BF008F058ED89F58A95C1BE523C4B11295ED2D0F"),
                             QByteArray::fromHex("491B9A66D3ED4EF19F02180289D5B1A1C2596AE568540A95DC5244198A9B8869")};
    QVector<QByteArray> ivs{QByteArray::fromHex("00000000000000000000000000000000"),
                            QByteArray::fromHex("1B1FE8F5A911CD4C0D800EDCE8ED0A94"),
                            QByteArray::fromHex("F0BCF70D7BB382917B1A9DAFBB0F38C3"),
                            QByteArray::fromHex("F66C06ED112BE4FA491A6BE4ECE2BD52"),
                            QByteArray::fromHex("54D483731064E5D6A082E09536D53EA4")};
    QVector<QByteArray> plainTexts{QByteArray::fromHex("2CBA6271A1044F90C30BA8FE91E1C163"),
                                   QByteArray::fromHex("05F05148EF495836AB0DA226B2E9D0C2"),
                                   QByteArray::fromHex("A792AC61E7110C434BC2BBCAB6E53CAE"),
                                   QByteArray::fromHex("4C81F5BDC1081170FF96F50B1F76A566"),
                                   QByteArray::fromHex("BD959F5B787037631A37051EA5F369F8")};
    QVector<QByteArray> cipherTexts{QByteArray::fromHex("00000000000000000000000000000000"),
                                    QByteArray::fromHex("2CBA6271A1044F90C30BA8FE91E1C163"),
                                    QByteArray::fromHex("05F05148EF495836AB0DA226B2E9D0C2"),
                                    QByteArray::fromHex("A792AC61E7110C434BC2BBCAB6E53CAE"),
                                    QByteArray::fromHex("4C81F5BDC1081170FF96F50B1F76A566")};

    SymmetricCipher cipher;

    for (int i = 0; i < keys.size(); ++i) {
        QVERIFY(cipher.init(SymmetricCipher::Twofish_CBC, SymmetricCipher::Decrypt, keys[i], ivs[i]));
        QByteArray ctNext = cipherTexts[i];
        QByteArray ptCur;
        QCOMPARE(cipher.blockSize(cipher.mode()), 16);
        for (int j = 0; j < 5000; ++j) {
            if (!cipher.process(ctNext)) {
                break;
            }

            if (!cipher.process(ctNext)) {
                break;
            }
        }

        QCOMPARE(ctNext, plainTexts[i]);
    }
}

void TestSymmetricCipher::testSalsa20()
{
    // http://www.ecrypt.eu.org/stream/svn/viewcvs.cgi/ecrypt/trunk/submissions/salsa20/full/verified.test-vectors?logsort=rev&rev=210&view=markup

    QByteArray key = QByteArray::fromHex("F3F4F5F6F7F8F9FAFBFCFDFEFF000102030405060708090A0B0C0D0E0F101112");
    QByteArray iv = QByteArray::fromHex("0000000000000000");

    SymmetricCipher cipher;
    QVERIFY(cipher.init(SymmetricCipher::Salsa20, SymmetricCipher::Encrypt, key, iv));

    QByteArray cipherTextA;
    for (int i = 0; i < 8; i++) {
        QByteArray data(64, '\0');
        QVERIFY(cipher.process(data));
        cipherTextA.append(data);
    }

    // Re-initialize
    QVERIFY(cipher.init(SymmetricCipher::Salsa20, SymmetricCipher::Encrypt, key, iv));

    QByteArray cipherTextB(512, '\0');
    QVERIFY(cipher.process(cipherTextB));
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

void TestSymmetricCipher::testChaCha20()
{
    // https://tools.ietf.org/html/draft-agl-tls-chacha20poly1305-04#section-7
    {
        QByteArray key = QByteArray::fromHex("0000000000000000000000000000000000000000000000000000000000000000");
        QByteArray iv = QByteArray::fromHex("0000000000000000");
        SymmetricCipher cipher;
        QByteArray data(64, 0);
        QVERIFY(cipher.init(SymmetricCipher::ChaCha20, SymmetricCipher::Encrypt, key, iv));
        QVERIFY(cipher.process(data));
        QCOMPARE(data,
                 QByteArray::fromHex("76b8e0ada0f13d90405d6ae55386bd28bdd219b8a08ded1aa836efcc8b770dc7da41597c5157488d7"
                                     "724e03fb8d84a376a43b8f41518a11cc387b669b2ee6586"));
    }

    {
        QByteArray key = QByteArray::fromHex("0000000000000000000000000000000000000000000000000000000000000001");
        QByteArray iv = QByteArray::fromHex("0000000000000000");
        SymmetricCipher cipher;
        QByteArray data(64, 0);
        QVERIFY(cipher.init(SymmetricCipher::ChaCha20, SymmetricCipher::Encrypt, key, iv));
        QVERIFY(cipher.process(data));
        QCOMPARE(data,
                 QByteArray::fromHex("4540f05a9f1fb296d7736e7b208e3c96eb4fe1834688d2604f450952ed432d41bbe2a0b6ea7566d2a"
                                     "5d1e7e20d42af2c53d792b1c43fea817e9ad275ae546963"));
    }

    {
        QByteArray key = QByteArray::fromHex("0000000000000000000000000000000000000000000000000000000000000000");
        QByteArray iv = QByteArray::fromHex("0000000000000001");
        SymmetricCipher cipher;
        QByteArray data(60, 0);
        QVERIFY(cipher.init(SymmetricCipher::ChaCha20, SymmetricCipher::Encrypt, key, iv));
        QVERIFY(cipher.process(data));
        QCOMPARE(data,
                 QByteArray::fromHex("de9cba7bf3d69ef5e786dc63973f653a0b49e015adbff7134fcb7df137821031e85a050278a708452"
                                     "7214f73efc7fa5b5277062eb7a0433e445f41e3"));
    }

    {
        QByteArray key = QByteArray::fromHex("0000000000000000000000000000000000000000000000000000000000000000");
        QByteArray iv = QByteArray::fromHex("0100000000000000");
        SymmetricCipher cipher;
        QByteArray data(64, 0);
        QVERIFY(cipher.init(SymmetricCipher::ChaCha20, SymmetricCipher::Encrypt, key, iv));
        QVERIFY(cipher.process(data));
        QCOMPARE(data,
                 QByteArray::fromHex("ef3fdfd6c61578fbf5cf35bd3dd33b8009631634d21e42ac33960bd138e50d32111e4caf237ee53ca"
                                     "8ad6426194a88545ddc497a0b466e7d6bbdb0041b2f586b"));
    }
}

void TestSymmetricCipher::testPadding()
{
    QByteArray key = QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
    QByteArray iv = QByteArray::fromHex("000102030405060708090a0b0c0d0e0f");
    QByteArray plainText = QByteArray::fromHex("6bc1bee22e409f96e93d");

    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);

    SymmetricCipherStream streamEnc(&buffer);
    QVERIFY(streamEnc.init(SymmetricCipher::Aes256_CBC, SymmetricCipher::Encrypt, key, iv));
    streamEnc.open(QIODevice::WriteOnly);
    streamEnc.write(plainText);
    streamEnc.close();
    buffer.reset();
    // make sure padding is written
    QCOMPARE(buffer.buffer().size(), 16);

    SymmetricCipherStream streamDec(&buffer);
    QVERIFY(streamDec.init(SymmetricCipher::Aes256_CBC, SymmetricCipher::Decrypt, key, iv));
    streamDec.open(QIODevice::ReadOnly);
    QByteArray decrypted = streamDec.readAll();
    QCOMPARE(decrypted, plainText);
}

void TestSymmetricCipher::testStreamReset()
{
    QByteArray key = QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
    QByteArray iv = QByteArray::fromHex("000102030405060708090a0b0c0d0e0f");

    QBuffer buffer;
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    SymmetricCipherStream writer(&buffer);
    QVERIFY(writer.init(SymmetricCipher::Aes256_CBC, SymmetricCipher::Encrypt, key, iv));
    QVERIFY(writer.open(QIODevice::WriteOnly));
    QCOMPARE(writer.write(QByteArray(4, 'Z')), qint64(4));
    // test if reset() and close() write only one block
    QVERIFY(writer.reset());
    QVERIFY(writer.reset());
    writer.close();
    QCOMPARE(buffer.buffer().size(), 16);
}
