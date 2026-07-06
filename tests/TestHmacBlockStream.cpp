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

#include <QTest>

QTEST_GUILESS_MAIN(TestHmacBlockStream)

void TestHmacBlockStream::initTestCase()
{
    QLocale::setDefault(QLocale::c());
    m_key = QByteArray(64, '\x42');
    m_data = QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
}

void TestHmacBlockStream::init()
{
    m_buffer = new QBuffer();
    QVERIFY(m_buffer->open(QIODevice::ReadWrite));
    m_raw = &(m_buffer->buffer());

    m_writer = new HmacBlockStream(m_buffer, m_key, 16);
    QVERIFY(m_writer->open(QIODevice::WriteOnly));

    m_reader = new HmacBlockStream(m_buffer, m_key);
    QVERIFY(m_reader->open(QIODevice::ReadOnly));
}

void TestHmacBlockStream::cleanup()
{
    delete m_writer;
    delete m_reader;
    delete m_buffer;
    m_buffer = nullptr;
    m_writer = nullptr;
    m_reader = nullptr;
}

void TestHmacBlockStream::testWriteRead()
{
    QCOMPARE(m_writer->write(m_data.left(16)), qint64(16));
    QVERIFY(m_writer->reset());
    m_buffer->reset();

    QCOMPARE(m_reader->read(17), m_data.left(16));
    QVERIFY(m_reader->atEnd());
}

void TestHmacBlockStream::testTamperData()
{
    QCOMPARE(m_writer->write(m_data.left(16)), qint64(16));
    QVERIFY(m_writer->reset());
    m_buffer->reset();

    (*m_raw)[data_offset] ^= 0xff;

    QByteArray result = m_reader->read(16);
    QVERIFY(result.isEmpty());
    QCOMPARE(m_reader->errorString(), QString("Mismatch between hash and data."));
    QVERIFY(m_reader->reset());
    m_buffer->reset();
    m_buffer->buffer().clear();
}

void TestHmacBlockStream::testTamperHmac()
{
    QCOMPARE(m_writer->write(m_data.left(16)), qint64(16));
    QVERIFY(m_writer->reset());
    m_buffer->reset();
    (*m_raw)[0] ^= 0xff;
    QByteArray result = m_reader->read(16);
    QVERIFY(result.isEmpty());
    QCOMPARE(m_reader->errorString(), QString("Mismatch between hash and data."));
    QVERIFY(m_reader->reset());
    m_buffer->reset();
    m_buffer->buffer().clear();
}

void TestHmacBlockStream::testTamperSize()
{
    QCOMPARE(m_writer->write(m_data.left(16)), qint64(16));
    QVERIFY(m_writer->reset());
    m_buffer->reset();
    (*m_raw)[hmac_field_size] += 1;
    QByteArray result = m_reader->read(16);
    QVERIFY(result.isEmpty());
    QCOMPARE(m_reader->errorString(), QString("Mismatch between hash and data."));
    QVERIFY(m_reader->reset());
    m_buffer->reset();
    m_buffer->buffer().clear();

    QCOMPARE(m_writer->write(m_data.left(16)), qint64(16));
    QVERIFY(m_writer->reset());
    m_buffer->reset();
    (*m_raw)[hmac_field_size + 3] += 1;
    result = m_reader->read(16);
    QVERIFY(result.isEmpty());
    QCOMPARE(m_reader->errorString(), QString("Block too short."));
    QVERIFY(m_reader->reset());
    m_buffer->reset();
    m_buffer->buffer().clear();
}
