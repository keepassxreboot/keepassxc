/*
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

#ifndef KEEPASSX_HMACBLOCKSTREAM_H
#define KEEPASSX_HMACBLOCKSTREAM_H

#include <QSysInfo>

#include "streams/LayeredStream.h"

class HmacBlockStream: public LayeredStream
{
Q_OBJECT

public:
    explicit HmacBlockStream(QIODevice* baseDevice, QByteArray key);
    HmacBlockStream(QIODevice* baseDevice, QByteArray key, qint32 blockSize);
    ~HmacBlockStream();

    bool reset() override;
    void close() override;

    static QByteArray getHmacKey(quint64 blockIndex, QByteArray key);

    bool atEnd() const override;

protected:
    qint64 readData(char* data, qint64 maxSize) override;
    qint64 writeData(const char* data, qint64 maxSize) override;

private:
    void init();
    bool readHashedBlock();
    bool writeHashedBlock();
    QByteArray getCurrentHmacKey() const;

    static const QSysInfo::Endian ByteOrder;
    qint32 m_blockSize;
    QByteArray m_buffer;
    QByteArray m_key;
    int m_bufferPos;
    quint64 m_blockIndex;
    bool m_eof;
    bool m_error;
};

#endif // KEEPASSX_HMACBLOCKSTREAM_H
