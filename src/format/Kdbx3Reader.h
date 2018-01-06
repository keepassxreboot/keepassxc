/*
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_KDBX3READER_H
#define KEEPASSX_KDBX3READER_H

#include <QCoreApplication>

#include "format/KeePass2Reader.h"
#include "keys/CompositeKey.h"

class Database;
class QIODevice;

class Kdbx3Reader: public BaseKeePass2Reader
{
Q_DECLARE_TR_FUNCTIONS(Kdbx3Reader)

public:
    Kdbx3Reader();

    using BaseKeePass2Reader::readDatabase;
    virtual Database* readDatabase(QIODevice* device, const CompositeKey& key, bool keepDatabase = false) override;

private:
    bool readHeaderField();

    void setCipher(const QByteArray& data);
    void setCompressionFlags(const QByteArray& data);
    void setMasterSeed(const QByteArray& data);
    void setTransformSeed(const QByteArray& data);
    void setTransformRounds(const QByteArray& data);
    void setEncryptionIV(const QByteArray& data);
    void setProtectedStreamKey(const QByteArray& data);
    void setStreamStartBytes(const QByteArray& data);
    void setInnerRandomStreamID(const QByteArray& data);

    QIODevice* m_device;
    QIODevice* m_headerStream;
    bool m_headerEnd;

    Database* m_db;
    QByteArray m_masterSeed;
    QByteArray m_encryptionIV;
    QByteArray m_streamStartBytes;
};

#endif // KEEPASSX_KDBX3READER_H
