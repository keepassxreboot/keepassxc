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

#include "TestHashedBlockStream.h"

#include <QBuffer>
#include <QTest>

#include "tests.h"
#include "crypto/Crypto.h"
#include "streams/HashedBlockStream.h"

QTEST_GUILESS_MAIN(TestHashedBlockStream)

void TestHashedBlockStream::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestHashedBlockStream::testWriteRead()
{
    QByteArray data = QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");

    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);

    HashedBlockStream writer(&buffer, 16);
    writer.open(QIODevice::WriteOnly);

    HashedBlockStream reader(&buffer);
    reader.open(QIODevice::ReadOnly);

    writer.write(data.left(16));
    QVERIFY(writer.reset());
    buffer.reset();
    QCOMPARE(reader.read(17), data.left(16));
    QVERIFY(reader.reset());
    buffer.reset();
    buffer.buffer().clear();

    writer.write(data.left(10));
    QVERIFY(writer.reset());
    buffer.reset();
    QCOMPARE(reader.read(5), data.left(5));
    QCOMPARE(reader.read(5), data.mid(5, 5));
    QCOMPARE(reader.read(1).size(), 0);
    QVERIFY(reader.reset());
    buffer.reset();
    buffer.buffer().clear();

    writer.write(data.left(20));
    QVERIFY(writer.reset());
    buffer.reset();
    QCOMPARE(reader.read(20), data.left(20));
    QCOMPARE(reader.read(1).size(), 0);
    QVERIFY(reader.reset());
    buffer.reset();
    buffer.buffer().clear();
}
