/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_KEEPASS1READER_H
#define KEEPASSX_KEEPASS1READER_H

#include <QtCore/QCoreApplication>

class Database;
class SymmetricCipherStream;
class QIODevice;

class KeePass1Reader
{
    Q_DECLARE_TR_FUNCTIONS(KeePass1Reader)

public:
    KeePass1Reader();
    Database* readDatabase(QIODevice* device, const QString& password,
                           const QByteArray& keyfileData);
    Database* readDatabase(const QString& filename, const QString& password,
                           const QByteArray& keyfileData);
    bool hasError();
    QString errorString();

private:
    enum PasswordEncoding
    {
        Windows1252,
        Latin1,
        UTF8
    };

    SymmetricCipherStream* testKeys(const QString& password, const QByteArray& keyfileData,
                                    qint64 contentPos);
    QByteArray key(const QByteArray& password, const QByteArray& keyfileData);
    bool verifyKey(SymmetricCipherStream* cipherStream);
    void raiseError(const QString& str);

    Database* m_db;
    QIODevice* m_device;
    quint32 m_encryptionFlags;
    QByteArray m_masterSeed;
    QByteArray m_encryptionIV;
    QByteArray m_contentHashHeader;
    QByteArray m_transformSeed;
    quint32 m_transformRounds;

    bool m_error;
    QString m_errorStr;
};

#endif // KEEPASSX_KEEPASS1READER_H
