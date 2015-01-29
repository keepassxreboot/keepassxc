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

#include "TestKeePass2RandomStream.h"

#include <QTest>

#include "tests.h"
#include "crypto/Crypto.h"
#include "crypto/CryptoHash.h"
#include "crypto/SymmetricCipher.h"
#include "format/KeePass2.h"
#include "format/KeePass2RandomStream.h"

QTEST_GUILESS_MAIN(TestKeePass2RandomStream)

void TestKeePass2RandomStream::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestKeePass2RandomStream::test()
{
    const QByteArray key("\x11\x22\x33\x44\x55\x66\x77\x88");
    const int Size = 128;


    SymmetricCipher cipher(SymmetricCipher::Salsa20, SymmetricCipher::Stream, SymmetricCipher::Encrypt,
                           CryptoHash::hash(key, CryptoHash::Sha256), KeePass2::INNER_STREAM_SALSA20_IV);

    const QByteArray data(QByteArray::fromHex("601ec313775789a5b7a7f504bbf3d228f443e3ca4d62b59aca84e990cacaf5c5"
                                              "2b0930daa23de94ce87017ba2d84988ddfc9c58db67aada613c2dd08457941a6"
                                              "1abc932417521ca24f2b0459fe7e6e0b090339ec0aa6faefd5ccc2c6f4ce8e94"
                                              "1e36b26bd1ebc670d1bd1d665620abf74f78a7f6d29809585a97daec58c6b050"));

    QByteArray cipherPad;
    cipherPad.fill('\0', Size);
    cipher.processInPlace(cipherPad);

    QByteArray cipherData;
    cipherData.resize(Size);

    for (int i = 0; i < Size; i++) {
        cipherData[i] = data[i] ^ cipherPad[i];
    }


    KeePass2RandomStream randomStream(key);
    QByteArray randomStreamData;
    randomStreamData.append(randomStream.process(data.mid(0, 7)));
    randomStreamData.append(randomStream.process(data.mid(7, 1)));
    QByteArray tmpData = data.mid(8, 12);
    randomStream.processInPlace(tmpData);
    randomStreamData.append(tmpData);
    randomStreamData.append(randomStream.process(data.mid(20, 44)));
    randomStreamData.append(randomStream.process(data.mid(64, 64)));


    SymmetricCipher cipherEncrypt(SymmetricCipher::Salsa20, SymmetricCipher::Stream, SymmetricCipher::Encrypt,
                                  CryptoHash::hash(key, CryptoHash::Sha256), KeePass2::INNER_STREAM_SALSA20_IV);
    QByteArray cipherDataEncrypt = cipherEncrypt.process(data);


    QCOMPARE(randomStreamData.size(), Size);
    QCOMPARE(cipherData, cipherDataEncrypt);
    QCOMPARE(randomStreamData, cipherData);
}
