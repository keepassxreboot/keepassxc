/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "TestHmacBlockStream.h"

#include "QBuffer"
#include <QTest>

#include "crypto/Crypto.h"
#include "streams/HmacBlockStream.h"

QTEST_GUILESS_MAIN(TestHmacBlockStream)

void TestHmacBlockStream::initTestCase()
{
    QVERIFY(Crypto::init());
    QLocale::setDefault(QLocale::c());
}

void TestHmacBlockStream::testWriteRead()
{
    QByteArray key(64, '\x42');
    QByteArray data = QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");

    QBuffer buffer;
    QVERIFY(buffer.open(QIODevice::ReadWrite));

    HmacBlockStream writer(&buffer, key, 16);
    QVERIFY(writer.open(QIODevice::WriteOnly));

    HmacBlockStream reader(&buffer, key);
    QVERIFY(reader.open(QIODevice::ReadOnly));

    QCOMPARE(writer.write(data.left(16)), qint64(16));
    QVERIFY(writer.reset());
    buffer.reset();

    QCOMPARE(reader.read(17), data.left(16));
    QVERIFY(reader.atEnd());
}

void TestHmacBlockStream::testTamperData()
{
    QByteArray key(64, '\x42');
    QByteArray data = QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");

    QBuffer buffer;
    QVERIFY(buffer.open(QIODevice::ReadWrite));

    HmacBlockStream writer(&buffer, key, 16);
    QVERIFY(writer.open(QIODevice::WriteOnly));

    HmacBlockStream reader(&buffer, key);
    QVERIFY(reader.open(QIODevice::ReadOnly));

    QCOMPARE(writer.write(data.left(16)), qint64(16));
    QVERIFY(writer.reset());
    buffer.reset();

    QByteArray& raw = buffer.buffer();
    const int hmac_size = 32;
    const int size_field = 4;
    const int data_offset = hmac_size + size_field;
    raw[data_offset] ^= 0xff;

    QByteArray result = reader.read(16);
    QVERIFY(result.isEmpty());
    QCOMPARE(reader.errorString(), QString("Mismatch between hash and data."));
    QVERIFY(reader.reset());
    buffer.reset();
    buffer.buffer().clear();
}

void TestHmacBlockStream::testTamperHmac()
{
    QByteArray key(64, '\x42');
    QByteArray data = QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");

    QBuffer buffer;
    QByteArray& raw = buffer.buffer();
    QVERIFY(buffer.open(QIODevice::ReadWrite));

    HmacBlockStream writer(&buffer, key, 16);
    QVERIFY(writer.open(QIODevice::WriteOnly));

    HmacBlockStream reader(&buffer, key);
    QVERIFY(reader.open(QIODevice::ReadOnly));

    QCOMPARE(writer.write(data.left(16)), qint64(16));
    QVERIFY(writer.reset());
    buffer.reset();
    raw[0] ^= 0xff;
    QByteArray result = reader.read(16);
    QVERIFY(result.isEmpty());
    QCOMPARE(reader.errorString(), QString("Mismatch between hash and data."));
    QVERIFY(reader.reset());
    buffer.reset();
    buffer.buffer().clear();
}

void TestHmacBlockStream::testTamperSize()
{
    QByteArray key(64, '\x42');
    QByteArray data = QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");

    QBuffer buffer;
    QByteArray& raw = buffer.buffer();
    QVERIFY(buffer.open(QIODevice::ReadWrite));

    HmacBlockStream writer(&buffer, key, 16);
    QVERIFY(writer.open(QIODevice::WriteOnly));

    HmacBlockStream reader(&buffer, key);
    QVERIFY(reader.open(QIODevice::ReadOnly));

    QCOMPARE(writer.write(data.left(16)), qint64(16));
    QVERIFY(writer.reset());
    buffer.reset();
    const int hmac_size = 32;
    raw[hmac_size] += 1;
    QByteArray result = reader.read(16);
    QVERIFY(result.isEmpty());
    QCOMPARE(reader.errorString(), QString("Mismatch between hash and data."));
    QVERIFY(reader.reset());
    buffer.reset();
    buffer.buffer().clear();

    QCOMPARE(writer.write(data.left(16)), qint64(16));
    QVERIFY(writer.reset());
    buffer.reset();
    raw[hmac_size + 3] += 1;
    result = reader.read(16);
    QVERIFY(result.isEmpty());
    QCOMPARE(reader.errorString(), QString("Block too short."));
    QVERIFY(reader.reset());
    buffer.reset();
    buffer.buffer().clear();
}
