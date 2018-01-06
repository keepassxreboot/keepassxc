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

#ifndef KEEPASSX_KDBX4READER_H
#define KEEPASSX_KDBX4READER_H

#include <QCoreApplication>
#include <QHash>
#include <QString>
#include <QByteArray>

#include "format/KeePass2.h"
#include "format/KeePass2Reader.h"
#include "crypto/SymmetricCipher.h"
#include "keys/CompositeKey.h"

class Database;
class QIODevice;

class Kdbx4Reader : public BaseKeePass2Reader
{
    Q_DECLARE_TR_FUNCTIONS(Kdbx4Reader)

public:
    Kdbx4Reader();

    using BaseKeePass2Reader::readDatabase;
    virtual Database* readDatabase(QIODevice* device, const CompositeKey& key, bool keepDatabase = false) override;

    QHash<QString, QByteArray> binaryPool();

private:
    bool readHeaderField(QIODevice* device);
    bool readInnerHeaderField(QIODevice* device);
    QVariantMap readVariantMap(QIODevice* device);

    void setCipher(const QByteArray& data);
    void setCompressionFlags(const QByteArray& data);
    void setMasterSeed(const QByteArray& data);
    void setEncryptionIV(const QByteArray& data);
    void setProtectedStreamKey(const QByteArray& data);
    void setInnerRandomStreamID(const QByteArray& data);

    QIODevice* m_device;

    Database* m_db;
    QByteArray m_masterSeed;
    QByteArray m_encryptionIV;
    QHash<QString, QByteArray> m_binaryPool;
};

#endif // KEEPASSX_KDBX4READER_H
