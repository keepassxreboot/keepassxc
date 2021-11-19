/*
 * Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or (at your option)
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSXC_KDBXREADER_H
#define KEEPASSXC_KDBXREADER_H

#include "KeePass2.h"

#include <QCoreApplication>
#include <QPointer>

class CompositeKey;
class Database;
class QIODevice;
class StoreDataStream;

/**
 * Abstract KDBX reader base class.
 */
class KdbxReader
{
    Q_DECLARE_TR_FUNCTIONS(KdbxReader)

public:
    KdbxReader() = default;
    virtual ~KdbxReader() = default;

    static bool readMagicNumbers(QIODevice* device, quint32& sig1, quint32& sig2, quint32& version);
    bool readDatabase(QIODevice* device, QSharedPointer<const CompositeKey> key, Database* db);

    bool hasError() const;
    QString errorString() const;

    KeePass2::ProtectedStreamAlgo protectedStreamAlgo() const;

protected:
    /**
     * Concrete reader implementation for reading database from device.
     *
     * @param device input device at the payload starting position
     * @param KDBX header data as bytes
     * @param key database encryption composite key
     * @param db database to read into
     * @return true on success
     */
    virtual bool readDatabaseImpl(QIODevice* device,
                                  const QByteArray& headerData,
                                  QSharedPointer<const CompositeKey> key,
                                  Database* db) = 0;

    /**
     * Read next header field from stream.
     *
     * @param headerStream input header stream
     * @param database to read header field for
     * @return true if there are more header fields
     */
    virtual bool readHeaderField(StoreDataStream& headerStream, Database* db) = 0;

    virtual void setCipher(const QByteArray& data);
    virtual void setCompressionFlags(const QByteArray& data);
    virtual void setMasterSeed(const QByteArray& data);
    virtual void setTransformSeed(const QByteArray& data);
    virtual void setTransformRounds(const QByteArray& data);
    virtual void setEncryptionIV(const QByteArray& data);
    virtual void setProtectedStreamKey(const QByteArray& data);
    virtual void setStreamStartBytes(const QByteArray& data);
    virtual void setInnerRandomStreamID(const QByteArray& data);

    void raiseError(const QString& errorMessage);

    QByteArray m_masterSeed;
    QByteArray m_encryptionIV;
    QByteArray m_streamStartBytes;
    QByteArray m_protectedStreamKey;
    KeePass2::ProtectedStreamAlgo m_irsAlgo = KeePass2::ProtectedStreamAlgo::InvalidProtectedStreamAlgo;

private:
    QPair<quint32, quint32> m_kdbxSignature;
    QPointer<Database> m_db;

    bool m_error = false;
    QString m_errorStr = "";
};

#endif // KEEPASSXC_KDBXREADER_H
