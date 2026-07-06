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

#ifndef KEEPASSX_TESTHMACBLOCKSTREAM_H
#define KEEPASSX_TESTHMACBLOCKSTREAM_H

#include <QBuffer>
#include <QObject>

#include "streams/HmacBlockStream.h"

class TestHmacBlockStream : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void testWriteRead();
    void testTamperData();
    void testTamperHmac();
    void testTamperSize();

private:
    QByteArray m_key;
    QByteArray m_data;
    QBuffer* m_buffer;
    QByteArray* m_raw;
    HmacBlockStream* m_writer;
    HmacBlockStream* m_reader;

    const int hmac_field_size = 32; // bytes
    const int size_field_size = 4; // bytes
    const int data_offset = hmac_field_size + size_field_size;
};

#endif // KEEPASSX_TESTHMACBLOCKSTREAM_H
